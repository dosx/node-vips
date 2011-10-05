// Copyright Erly Inc 2011, All Rights Reserved
// Authors: Walt Lin, Bo Wang

#include <string>

// Transform: resize and/or rotate an image.
//
// If 'cols' or 'rows' is < 0, no resizing is done.  Otherwise, the image is
// scaled down such that the aspect ratio is preserved.  If 'crop_to_size' is
// true, the resulting image will be the exact size of cols x rows (assuming
// it was larger than that), and the image will be cropped to reach it.
// If 'crop_to_size' is false, it is just scaled down such that the resulting
// image will fit into the cols x rows.
//
// 'rotate_degrees' must be one of 0, 90, 180, or 270.
//
// If 'auto_orient' is true, the orientation is read from EXIF data on the 
// image, and it is rotated to be right side up, and an orientation of '1'
// is written back to the EXIF.
//
// An optional ":compression_level" may be appended to 'dst_path' to set the
// jpeg compression level if dst_path is a jpeg.
//
// Return 0 on success, > 0 if an error and fill in 'err_msg'.
int DoTransform(int cols, int rows, bool crop_to_size,
                int rotate_degrees, bool auto_orient,
                const std::string& src_path, const std::string& dst_path,
                int* new_width, int* new_height, std::string* err_msg);
