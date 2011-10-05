// Copyright Erly Inc 2011, All Rights Reserved
// Authors: Bo Wang, Walt Lin

#include <string>

// Transform: resize and/or rotate an image.
//
// If 'cols' or 'rows' is < 0, no resizing is done.
// 'rotate_degrees' must be one of 0, 90, 180, or 270.
// If 'auto_orient' is true, the orientation is read from EXIF data on the 
// image, and it is rotated to be right side up, and an orientation of '1'
// is written back to the EXIF.
// An optional ":compression_level" may be appended to 'dst_path' to set the
// jpeg compression level if dst_path is a jpeg.
//
// Return 0 on success, > 0 if an error and fill in 'err_msg'.
int DoTransform(int cols, int rows, int rotate_degrees, bool auto_orient,
		const std::string& src_path, const std::string& dst_path,
		std::string* err_msg);
