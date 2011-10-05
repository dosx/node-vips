// Copyright Erly Inc 2011, All Rights Reserved
// Authors: Bo Wang, Walt Lin
//
// Note: we found im_shrink produces better looking resizes than im_affinei_all.
// 
// Note(bo): im_resize_linear is marked as deprecated in vips-7.26, 
//       This method locates at libvips/deprecated/im_resize_linear.c
//       The list of functions I've tried:
//        - im_subsample
//        - im_rightshift_size
//        - im_shrink
//        - im_affinei
//        - im_affinei_all
// if( im_resize_linear(img_in, img_out, data->cols, data->rows) ) {
//   printf("[error] failed to resize image %s\n", *src_path);
// }
//
// To compile a test program that uses this:
//  g++ test.cc src/transform.cc   `pkg-config --cflags --libs vips-7.26`  `pkg-config --cflags --libs exiv2`

#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <algorithm>
#include <string>

#include <vips/vips.h>
#include <exiv2/exiv2.hpp>

#include "transform.h"

// Free a VipsImage when it goes out of scope.
class ScopedImage {
 public:
  ScopedImage() : i_(NULL) {}
  ScopedImage(VipsImage* i) : i_(i) {}

  ~ScopedImage() {
    if (i_ != NULL) {
      g_object_unref(i_);
    }
  }

  VipsImage* operator->() { return i_; }
  VipsImage* get() { return i_; }

  void set(VipsImage* i) { i_ = i; }

 private:
  VipsImage* i_;
};

// Read EXIF data for image in 'path' and return the rotation needed to turn
// it right side up.  Return < 0 upon error, and fill in 'err'.
// TODO(walt): look at ifd0, not ifd1 (which applies to thumbnail).
static int GetEXIFRotationNeeded(const std::string& path, std::string* err) {
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
static int WriteEXIFOrientation(const std::string& path, uint16_t orientation) {
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

int DoTransform(int cols, int rows, int rotate_degrees, bool auto_orient,
		const std::string& src_path, const std::string& dst_path,
		std::string* err_msg) {
  g_type_init();  // ok to call multiple times?

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

  ScopedImage in(vips_image_new_mode(src_path.c_str(), "r"));
  if (in.get() == NULL) {
    err_msg->assign("could not open input");
    return 1;
  }

  ScopedImage out(vips_image_new_mode(dst_path.c_str(), "w"));
  if (out.get() == NULL) {
    err_msg->assign("could not open output");
    return 1;
  }

  ScopedImage tmp;
  if (need_resize && need_rotate) {
    tmp.set(vips_image_new());
    if (tmp.get() == NULL) {
      err_msg->assign("could not open temp glue image");
      return 1;
    }
  }

  if (need_resize) {
    double xshrink = static_cast<double>(in->Xsize) / std::max(cols, 1);
    double yshrink = static_cast<double>(in->Ysize) / std::max(rows, 1);
    if (im_shrink(in.get(), tmp.get() ? tmp.get() : out.get(),
		  xshrink, yshrink)) {
      err_msg->assign("resize failed: ");
      err_msg->append(vips_error_buffer());
      return 1;
    }
  }

  if (need_rotate) {
    int (*f)(VipsImage *, VipsImage *) = rotate_degrees == 90 ? im_rot90 :
      rotate_degrees == 180 ? im_rot180 : im_rot270;
    if ((*f)(tmp.get() ? tmp.get() : in.get(), out.get())) {
      err_msg->assign("rotate failed");
      return 1;
    }
  }

  if (auto_orient && need_rotate) {
    if (WriteEXIFOrientation(dst_path, 1 /* orientation */) < 0) {
      err_msg->assign("failed to write new EXIF orientation");
      return 1;
    }
  }

  return 0;
}
