// Copyright Erly Inc 2011, All Rights Reserved
// Authors: Walt Lin, Bo Wang
//
// Loosely based on vipsthumbnail.c
//
// To compile a test program that uses this:
//  g++ test.cc src/transform.cc   `pkg-config --cflags --libs vips-7.26`  `pkg-config --cflags --libs exiv2`

#include <ctype.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <algorithm>
#include <string>
#include <vector>

#include <vips/vips.h>
#include <exiv2/exiv2.hpp>

#include "transform.h"

#define DEBUG 0

using std::string;

static const char kJpegCompressionFactor[] = ":92";

string SimpleItoa(int x) {
  char buf[16];
  snprintf(buf, sizeof(buf), "%d", x);
  return string(buf);
}

// Free VipsImages when this object goes out of scope.
class ImageFreer {
 public:
  ImageFreer() {}

  ~ImageFreer() {
    for (int i = 0; i < v_.size(); i++) {
      if (v_[i] != NULL) {
        g_object_unref(v_[i]);
      }
    }
    v_.clear();
  }

  void add(VipsImage* i) { v_.push_back(i); }

 private:
  std::vector<VipsImage*> v_;
};

// Read EXIF data for image in 'path' and return the rotation needed to turn
// it right side up.  Return < 0 upon error, and fill in 'err'.
// TODO(walt): look at IFD0, not IFD1 (which applies to thumbnail).
static int GetEXIFRotationNeeded(const string& path, string* err) {
  int orientation = 0;
  try {
    Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(path);
    assert(image.get() != 0);
    image->readMetadata();
    Exiv2::ExifData &ed = image->exifData();
    Exiv2::ExifData::const_iterator it = Exiv2::orientation(ed);
    if (it != ed.end() && it->count() == 1) {
      orientation = it->toLong();
    } else {
      return 0;
    }
  } catch (Exiv2::Error& e) {
    err->assign("exiv2 error: ");
    err->append(e.what());
    return -1;
  }

  // We only expect values of 1, 3, 6, 8, see
  // http://www.impulseadventure.com/photo/exif-orientation.html
  switch (orientation) {
  case 1:    return 0;
  case 3:    return 180;
  case 6:    return 90;
  case 8:    return 270;
  default:
    err->assign("unexpected orientation value");
    return -1;
  }
}

// Write a new value to the EXIF orientation tag for the image in 'path'.
// Return 0 on success.
static int WriteEXIFOrientation(const string& path, uint16_t orientation) {
  try {
    Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(path);
    assert(image.get() != 0);
    image->readMetadata();
    image->exifData()["Exif.Image.Orientation"] = orientation;
    image->writeMetadata();
    return 0;
  } catch (Exiv2::Error& e) {
    printf("exiv2 error writing orientation: %s\n", e.what());
    return -1;
  }
}

// Calculate shrink factors: return an integer factor to shrink by ( >= 1 ),
// as well as a residual [0,1], so we can shrink in two stages.
static int CalculateShrink(int width, int height,
                           int new_width, int new_height,
                           bool crop, double *residual) {
  double xf = static_cast<double>(width) / std::max(new_width, 1);
  double yf = static_cast<double>(height) / std::max(new_height, 1);
  double factor = crop ? std::min(xf, yf) : std::max(xf, yf);
  factor = std::max(factor, 1.0);
  int shrink = floor(factor);
  if (residual != NULL) {
    *residual = shrink / factor;
  }
  return shrink;
}

// Resize the image, maintaining aspect ratio.  If 'crop' is true, the
// image will be scaled down until one dimension reaches the box and
// then the image will be cropped to reach the exact dimensions
// (keeping it centered); otherwise, the image will be scaled until it
// fits inside the requested box.  Allocate a new VipsImage and return
// it if successful; it will be local to 'in'.
//
// TODO(walt): add sharpening?
static VipsImage* ResizeAndCrop(VipsImage* in, int new_x, int new_y,
                                bool crop) {
  VipsImage* x = in;
  VipsImage* t[4];
  if (im_open_local_array(in, t, 4, "scratch", "p")) {
    return NULL;
  }

  double residual;
  int shrink = CalculateShrink(x->Xsize, x->Ysize, new_x, new_y,
                               crop, &residual);

  if (DEBUG) {
    fprintf(stderr, "resizing image from %dx%d to %dx%d, "
            "crop=%d, shrink=%d, residual scale=%f\n",
            x->Xsize, x->Ysize, new_x, new_y, crop, shrink, residual);
  }
  
  // First, shrink an integral amount with im_shrink.  Then, do the leftover
  // part with im_affinei using bilinear interpolation.
  VipsInterpolate* interp = vips_interpolate_bilinear_static();
  if (im_shrink(x, t[0], shrink, shrink) ||
      im_affinei_all(t[0], t[1], interp, residual, 0, 0, residual, 0, 0)) {
    return NULL;
  }
  x = t[1];

  if (crop) {
    int width = std::min(x->Xsize, new_x);
    int height = std::min(x->Ysize, new_y);
    int left = (x->Xsize - width + 1) / 2;
    int top = (x->Ysize - height + 1) / 2;
    if (DEBUG) {
      fprintf(stderr, "cropping from %dx%d to %dx%d\n",
              x->Xsize, x->Ysize, new_x, new_y);
    }
    if (im_extract_area(x, t[2], left, top, width, height)) {
      return NULL;
    }
    x = t[2];
  }

  return x;
}

// Rotate the image, returning a new VipsImage that is allocated local
// to 'in'.  Return NULL if there is an error.
VipsImage* Rotate(VipsImage* in, int degrees) {
  VipsImage* tmp = vips_image_new();
  if (tmp == NULL) {
    return NULL;
  }
  vips_object_local(in, tmp);

  int r = 0;
  switch (degrees) {
    case 0:   return in;
    case 90:  r = im_rot90(in, tmp);   break;
    case 180: r = im_rot180(in, tmp);  break;
    case 270: r = im_rot270(in, tmp);  break;
    default:  return NULL;
  }
  return r ? NULL : tmp;
}

// Return the path to open the source image with.  libjpeg supports
// fast shrink-on-read, so load a lower resolution version if it's a
// jpeg.
static string GetSourcePathWithOptions(const string& p, const string& format,
                                       int new_x, int new_y, bool crop) {
  if (format != "jpeg") {
    return p;
  }

  // This just reads the header, it's fast.
  VipsImage* im = vips_image_new_from_file(p.c_str());
  if (im == NULL) {
    return p;
  }
  
  int shrink = CalculateShrink(im->Xsize, im->Ysize, new_x, new_y, crop, NULL);
  g_object_unref(im);
  shrink = shrink > 8 ? 8 : shrink > 4 ? 4 : shrink > 2 ? 2 : 1;
  string path = p;
  path.append(":" + SimpleItoa(shrink));
  
  if (DEBUG) {
    fprintf(stderr, "using fast jpeg shrink, factor %d\n", shrink);
  }
  
  return path;
}

// Return the path to open the target image with.  If the image is a
// jpeg, add jpeg compression factor options.
static string GetDestPathWithOptions(const string& p, const string& format) {
  if (format == "jpeg") {
    string str = p;
    str.append(kJpegCompressionFactor);
    return str;
  } else {
    return p;
  }
}

int DoTransform(int cols, int rows, bool crop_to_size,
		int rotate_degrees, bool auto_orient,
		const string& src_path, const string& dst_path,
                int* new_width, int* new_height, string* err_msg) {
  ImageFreer freer;

  if (src_path == dst_path) {
    err_msg->assign("dest path cannot be same as source path");
    return -1;
  }

  if (auto_orient && rotate_degrees != 0) {
    err_msg->assign("can't rotate and auto-orient");
    return -1;
  }

  VipsFormatClass* format = vips_format_for_file(src_path.c_str());
  if (format == NULL) {
    err_msg->assign("bad format");
    return -1;
  }

  string imgformat = VIPS_OBJECT_CLASS(format)->nickname;
  if (imgformat != "jpeg" && imgformat != "png" && imgformat != "gif") {
    err_msg->assign("unsupported image format ");
    err_msg->append(imgformat);
    return -1;
  }

  // If auto-orienting, find how much we need to rotate.
  if (auto_orient) {
    int r = GetEXIFRotationNeeded(src_path.c_str(), err_msg);
    if (r < 0) {
      return -1;
    }
    rotate_degrees = r;
  }

  if (rotate_degrees != 0 && rotate_degrees != 90 &&
      rotate_degrees != 180 && rotate_degrees != 270) {
    err_msg->assign("illegal rotate_degrees");
    return -1;
  }

  // Open the input and output images.
  VipsImage *in, *out;
  {
    string p = GetSourcePathWithOptions(src_path, imgformat, cols, rows,
                                        crop_to_size);
    in = vips_image_new_mode(p.c_str(), "rd");
    if (in == NULL) {
      err_msg->assign("could not open input: ");
      err_msg->append(vips_error_buffer());
      return -1;
    }
    freer.add(in);

    p = GetDestPathWithOptions(dst_path, imgformat);
    out = vips_image_new_mode(p.c_str(), "w");
    if (out == NULL) {
      err_msg->assign("could not open output: ");
      err_msg->append(vips_error_buffer());
      return -1;
    }
    
    vips_object_local(in, out);
  }

  // Resize and/or crop.
  VipsImage* img = in;
  if (cols > 0 && rows > 0) {
    img = ResizeAndCrop(img, cols, rows, crop_to_size);
    if (img == NULL) {
      err_msg->assign("resize and crop failed: ");
      err_msg->append(vips_error_buffer());
      return -1;
    }
  }

  // Rotate.
  if (rotate_degrees > 0) {
    img = Rotate(img, rotate_degrees);
    if (img == NULL) {
      err_msg->assign("rotate failed: ");
      err_msg->append(vips_error_buffer());
      return -1;
    }
  }

  // Write the image.
  if (im_copy(img, out)) {
    err_msg->assign("copy failed");
    return -1;
  }

  // Write new EXIF orientation.
  if (auto_orient && rotate_degrees > 0) {
    if (WriteEXIFOrientation(dst_path, 1 /* orientation */) < 0) {
      err_msg->assign("failed to write new EXIF orientation");
      return -1;
    }
  }

  if (new_width != NULL) *new_width = img->Xsize;
  if (new_height != NULL) *new_height = img->Ysize;

  return 0;
}

void InitTransform(const char* argv0) {
  assert(vips_init(argv0) == 0);
}
