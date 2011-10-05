// Copyright Erly Inc 2011, All Rights Reserved
// Authors: Walt Lin, Bo Wang
//
// Note: we found im_shrink produces better looking resizes than im_affinei.
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

VipsImage* NewGlueImage(ImageFreer* freer, string* err_msg) {
  VipsImage* i = vips_image_new();
  if (i == NULL) {
    err_msg->assign("could not open temp image");
  } else {
    freer->add(i);
  }
  return i;
}

// Read EXIF data for image in 'path' and return the rotation needed to turn
// it right side up.  Return < 0 upon error, and fill in 'err'.
// TODO(walt): look at ifd0, not ifd1 (which applies to thumbnail).
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

static string GetDestPathWithOptions(const string& p) {
  // Add jpeg compression factor to end of dest path.
  int dotpos = p.rfind('.');
  if (dotpos != string::npos) {
    string ext = p.substr(dotpos + 1);
    if (ext == "jpg" || ext == "jpeg") {
      string str = p;
      str.append(kJpegCompressionFactor);
      return str;
    }
  }
  return p;
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

int DoTransform(int cols, int rows, bool crop_to_size,
		int rotate_degrees, bool auto_orient,
		const string& src_path,
                const string& dst_path,
                int* new_width, int* new_height, 
		string* err_msg) {
  g_type_init();  // ok to call multiple times?

  if (src_path == dst_path) {
    err_msg->assign("dest path cannot be same as source path");
    return 1;
  }

  // If auto-orienting, find how much we need to rotate.
  if (auto_orient) {
    if (rotate_degrees != 0) {
      err_msg->assign("can't rotate and auto-orient");
      return 1;
    }
    
    int r = GetEXIFRotationNeeded(src_path.c_str(), err_msg);
    if (r < 0) {
      return 1;
    }
    rotate_degrees = r;
  }

  bool need_resize = cols > 0 && rows > 0;
  bool need_rotate = rotate_degrees > 0;

  if (!need_resize && !need_rotate) {
    err_msg->assign("nothing to do");
    return 1;
  }

  if (rotate_degrees != 0 && rotate_degrees != 90 &&
      rotate_degrees != 180 && rotate_degrees != 270) {
    err_msg->assign("illegal rotate_degrees");
    return 1;
  }

  ImageFreer freer;
  VipsImage* in = vips_image_new_mode(src_path.c_str(), "r");
  freer.add(in);
  if (in == NULL) {
    err_msg->assign("could not open input: ");
    err_msg->append(vips_error_buffer());
    return 1;
  }
  
  VipsImage* cur = in;

  // Resize image.
  if (need_resize) {
    // Shrink to the larger dimension if 'crop_to_size' is true; otherwise,
    // shrink to the smaller dimension.
    double xs = static_cast<double>(cur->Xsize) / std::max(cols, 1);
    double ys = static_cast<double>(cur->Ysize) / std::max(rows, 1);
    double shrink = crop_to_size ? std::min(xs, ys) : std::max(xs, ys);

    if (DEBUG) {
      fprintf(stderr, "resizing; orig size %dx%d, requested size %dx%d, "
              "crop_to_size=%d, xshrink=%f yshrink=%f shrink=%f\n",
              cur->Xsize, cur->Ysize, cols, rows, crop_to_size, xs, ys, shrink);
    }

    if (shrink >= 1.0) {
      VipsImage* img = NewGlueImage(&freer, err_msg);
      if (img == NULL || im_shrink(cur, img, shrink, shrink)) {
        err_msg->assign("resize failed: ");
        err_msg->append(vips_error_buffer());
        return 1;
      }
      
      cur = img;
    }

    // Crop if necessary.
    if (crop_to_size && fabs(xs - ys) > 0.001) {
      int width = std::min(cur->Xsize, cols);
      int height = std::min(cur->Ysize, rows);
      int left = (cur->Xsize - width + 1) / 2;
      int top = (cur->Ysize - height + 1) / 2;

      if (DEBUG) {
        fprintf(stderr, "size %dx%d, extract left=%d top=%d "
                "width=%d height=%d\n",
                cur->Xsize, cur->Ysize, left, top, width, height);
      }
      
      VipsImage* img = NewGlueImage(&freer, err_msg);
      if (img == NULL ||
          im_extract_area(cur, img, left, top, width, height)) {
        err_msg->assign("extract failed: " );
        err_msg->append(vips_error_buffer());
        return 1;
      }

      cur = img;
    }
  }

  // Rotate image.
  if (need_rotate) {
    VipsImage* img = NewGlueImage(&freer, err_msg);
    if (img == NULL) {
      return 1;
    }
    
    int (*f)(VipsImage *, VipsImage *) = rotate_degrees == 90 ? im_rot90 :
      rotate_degrees == 180 ? im_rot180 : im_rot270;
    if ((*f)(cur, img)) {
      err_msg->assign("rotate failed");
      return 1;
    }

    cur = img;
  }

  if (new_width != NULL) *new_width = cur->Xsize;
  if (new_height != NULL) *new_height = cur->Ysize;

  // Write the image.
  {
    string p = GetDestPathWithOptions(dst_path);
    if (vips_image_write(cur, p.c_str())) {
      err_msg->assign("could not write new image: ");
      err_msg->append(vips_error_buffer());
      return 1;
    }
  }

  // Write new EXIF orientation.
  if (auto_orient && need_rotate) {
    if (WriteEXIFOrientation(dst_path, 1 /* orientation */) < 0) {
      err_msg->assign("failed to write new EXIF orientation");
      return 1;
    }
  }

  return 0;
}
