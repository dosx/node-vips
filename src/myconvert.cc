// Copyright Erly Inc 2011, All Rights Reserved
// Author: Walt Lin
//
// To compile: g++ -o myconvert  src/myconvert.cc src/transform.cc   `pkg-config --cflags --libs exiv2 vips-7.26`

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "transform.h"

void Usage() {
  printf("usage: myconvert input output command [options]\n"
	 "command is one of:\n"
	 "  resize (options: width height crop_to_size auto_orient)\n"
	 "  rotate (options: degrees)\n"
   "  autorotate ;   rotates according to exif and strips exif\n");
}

int main(int argc, char **argv) {
  InitTransform(argv[0]);

  if (argc < 4) {
    Usage();
    return 1;
  }

  if (strcmp(argv[3], "resize") == 0) {
    if (argc != 8) {
      Usage();
      return 1;
    }

    errno = 0;
    int width = strtol(argv[4], NULL, 10);
    int height = strtol(argv[5], NULL, 10);
    bool crop_to_size = strtol(argv[6], NULL, 10);
    bool auto_orient = strtol(argv[7], NULL, 10);
    if (errno != 0 || width <= 0 || height <= 0) {
      Usage();
      return 1;
    }

    std::string err;
    if (DoTransform(width, height, crop_to_size, 0, auto_orient,
		    argv[1], argv[2], NULL, NULL, &err)) {
      printf("resize failed: %s\n", err.c_str());
      return 1;
    }
  } else if (strcmp(argv[3], "rotate") == 0) {
    if (argc != 5) {
      Usage();
      return 1;
    }

    errno = 0;
    int degrees = strtol(argv[4], NULL, 10);
    if (degrees == 0 && errno != 0) {
      Usage();
      return 1;
    }

    std::string err;
    if (DoTransform(-1, -1, false, degrees, false,
		    argv[1], argv[2], NULL, NULL, &err)) {
      printf("rotate failed: %s\n", err.c_str());
      return 1;
    }
  } else if (strcmp(argv[3], "autorotate") == 0) {
    if (argc != 4) {
      Usage();
      return 1;
    }

    std::string err;
    if (DoTransform(-1, -1, false, 0, true /* auto-orient */,
                    argv[1], argv[2], NULL, NULL, &err)) {
      printf("autorotate failed: %s\n", err.c_str());
      return 1;
    }
  } else {
    Usage();
    return 1;
  }

  return 0;
}
