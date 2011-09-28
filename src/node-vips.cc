#include <node.h>
#include <node_version.h>
#include <node_buffer.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <vips/vips.h>

using namespace v8;
using namespace node;

namespace {
/*
struct ResizeData {
  Persistent<Object>   buffer;
  Persistent<String>   path;
  Persistent<Function> cb;
  int cols;
  int rows;
};
*/


struct ResizeData {
  Persistent<String>   src_file;
  Persistent<String>   dst_file;
  Persistent<Function> cb;
  int cols;
  int rows;
};

int resize (eio_req *req) {

  ResizeData *data = (ResizeData*) req->data;

/*
  void* img  = Buffer::Data(data->buffer);
  int length = Buffer::Length(data->buffer);
  String::Utf8Value path_str(data->path);
  printf("output = %s\n", *path_str);
*/

  String::Utf8Value src_path(data->src_file);
  String::Utf8Value dst_path(data->dst_file);

  IMAGE *img_in  = im_open(*src_path, "r");

  IMAGE *img_out = im_open(*dst_path, "w");

  // Note: im_resize_linear is marked as deprecated in vips-7.26, 
  //       it is easier to use.
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

  double xshrink = (double)img_in->Xsize / (double)data->cols;
  double yshrink = (double)img_in->Ysize / (double)data->rows;
  if( im_shrink(img_in, img_out, xshrink, yshrink) ) {
    printf("[error] failed to resize image %s\n", *src_path);
  }

  if( im_close(img_in) ) {
    printf("[error] failed to close image %s\n", *src_path);
  }

  if( im_close(img_out) ) {
    printf("[error] failed to close image %s\n", *dst_path);
  }

  return 0;
}

int resize_callback (eio_req *req) {

  HandleScope scope;

  ev_unref(EV_DEFAULT_UC);
  
  ResizeData *data = (ResizeData*)req->data;
  
  Local<Value> argv[2];
  argv[0] = Local<Value>::New(Null());
  argv[1] = Integer::New(req->result);

  TryCatch try_catch;

  data->cb->Call(Context::GetCurrent()->Global(), 2, argv);
  
  if (try_catch.HasCaught()) {
    FatalException(try_catch);
  }
  
  // data->buffer.Dispose();
  // data->path.Dispose();
  data->src_file.Dispose();
  data->dst_file.Dispose();
  data->cb.Dispose();
  
  delete data;
  
  return 0;
}

Handle<Value> Resize(const Arguments& args) {

  HandleScope scope;
/*
  if (args.Length() < 5) {
      return ThrowException(Exception::Error(
        String::New("Not enough parameters: resize(image_buffer, new_x_pixels, new_y_pixels, output_path, callback)")
      ));
  } else if (!args[1]->IsNumber() || !args[2]->IsNumber()) {
      return ThrowException(Exception::Error(
        String::New("In resize(image_buffer, new_x_pixels, new_y_pixels, output_path, callback), new_x_pixels and new_y_pixels must be Number.")
      ));
  } else if (!Buffer::HasInstance(args[0])) {
    return ThrowException(Exception::Error(
      String::New("In resize(image_buffer, new_x_pixels, new_y_pixels, output_path, callback), image_buffer must be a Buffer.")
    ));
  } else if (!args[3]->IsString()) {
    return ThrowException(Exception::Error(
      String::New("In resize(image_buffer, new_x_pixels, new_y_pixels, output_path, callback), output_path must be a String.")
    ));
  } else if (!args[4]->IsFunction()) {
    return ThrowException(Exception::Error(
      String::New("In resize(image_buffer, new_x_pixels, new_y_pixels, output_path, callback), callback must be a Function.")
    ));
  }
*/

  if (args.Length() < 5) {
      return ThrowException(Exception::Error(
        String::New("Not enough parameters: resize(input_path, new_x_pixels, new_y_pixels, output_path, callback)")
      ));
  } else if (!args[0]->IsString()) {
    return ThrowException(Exception::Error(
      String::New("In resize(input_path, new_x_pixels, new_y_pixels, output_path, callback), input_path must be a Buffer.")
    ));
  } else if (!args[1]->IsNumber() || !args[2]->IsNumber()) {
      return ThrowException(Exception::Error(
        String::New("In resize(input_path, new_x_pixels, new_y_pixels, output_path, callback), new_x_pixels and new_y_pixels must be Number.")
      ));
  } else if (!args[3]->IsString()) {
    return ThrowException(Exception::Error(
      String::New("In resize(input_path, new_x_pixels, new_y_pixels, output_path, callback), output_path must be a String.")
    ));
  } else if (!args[4]->IsFunction()) {
    return ThrowException(Exception::Error(
      String::New("In resize(input_path, new_x_pixels, new_y_pixels, output_path, callback), callback must be a Function.")
    ));
  }


  ResizeData *resize_data = new ResizeData();

/*
  // Make a Persistent point to buffer and path
  Persistent<Object> buffer = Persistent<Object>::New(args[0]->ToObject());
  Persistent<String> path = Persistent<String>::New(args[3]->ToString());
  int size = Buffer::Length(buffer);
  printf("size = %d\n", size);
*/
  // Create a persistent reference to the callback function,
  // otherwise we wouldn't be able to invoke the callback function
  // later outside the scope of this function
  Local<Function> cb = Local<Function>::Cast(args[4]);
  resize_data->cb    = Persistent<Function>::New(cb);

  int cols = args[1]->Int32Value();
  int rows = args[2]->Int32Value();

/*
  resize_data->buffer  = buffer;
  resize_data->path    = path;
*/
  resize_data->cols    = cols;
  resize_data->rows    = rows;

  Persistent<String> src = Persistent<String>::New(args[0]->ToString());
  Persistent<String> dst = Persistent<String>::New(args[3]->ToString());
  resize_data->src_file = src;
  resize_data->dst_file = dst;
/*
  eio_req *req = new eio_req();
  req->data = resize_data;
  resize(req);
*/

  eio_custom(resize, EIO_PRI_DEFAULT, resize_callback, resize_data);
  ev_ref(EV_DEFAULT_UC);

  return Undefined();
}

struct RotateData {
  Persistent<String>   src_file;
  Persistent<String>   dst_file;
  Persistent<Function> cb;
  int degrees;
};

int rotate (eio_req *req) {

  RotateData *data = (RotateData*) req->data;

  String::Utf8Value src_path(data->src_file);
  String::Utf8Value dst_path(data->dst_file);

  IMAGE *img_in  = im_open(*src_path, "r");

  IMAGE *img_out = im_open(*dst_path, "w");

  int degrees = data->degrees % 360;

  int error_code = 0;

  switch(degrees) {
    case 0  : break;
    case 90 : error_code = im_rot90 (img_in, img_out);
              break;
    case 180: error_code = im_rot180(img_in, img_out);
              break;
    case 270: error_code = im_rot270(img_in, img_out);
              break;
    default : printf("[warning] rotation skipped because of illegal degrees\n");
              break;
  }

  if(error_code) {
    printf("[error] encountered an error when rotating %s\n", *src_path);
  }

  if( im_close(img_in) ) {
    printf("[error] failed to close image %s\n", *src_path);
  }

  if( im_close(img_out) ) {
    printf("[error] failed to close image %s\n", *dst_path);
  }
  return 0;
}

int rotate_callback (eio_req *req) {

  HandleScope scope;

  ev_unref(EV_DEFAULT_UC);
  
  RotateData *data = (RotateData*)req->data;
  
  Local<Value> argv[2];
  argv[0] = Local<Value>::New(Null());
  argv[1] = Integer::New(req->result);

  TryCatch try_catch;

  data->cb->Call(Context::GetCurrent()->Global(), 2, argv);

  if (try_catch.HasCaught()) {
    FatalException(try_catch);
  }
  
  data->src_file.Dispose();
  data->dst_file.Dispose();
  data->cb.Dispose();
  
  delete data;
  
  return 0;
}

Handle<Value> Rotate(const Arguments& args) {
  HandleScope scope;

  if (args.Length() < 3) {
    return ThrowException(Exception::Error(
      String::New("Not enough parameters: rotate(input_path, degree, output_path, callback)")
    ));
  } else if (!args[0]->IsString()) {
    return ThrowException(Exception::Error(
      String::New("In rotate(input_path, degree, output_path, callback), input_path must be a Buffer.")
    ));
  } else if (!args[1]->IsNumber()) {
    return ThrowException(Exception::Error(
      String::New("In rotate(input_path, degree, output_path, callback), degree must be a Number.")
    ));
  } else if (!args[2]->IsString()) {
    return ThrowException(Exception::Error(
      String::New("In rotate(input_path, degree, output_path, callback), output_path must be a String.")
    ));
  } else if (!args[3]->IsFunction()) {
    return ThrowException(Exception::Error(
      String::New("In rotate(input_path, degree, output_path, callback), callback must be a Function.")
    ));
  }

  int degrees = args[1]->Int32Value();

  RotateData *rotate_data = new RotateData();

  Persistent<String> src = Persistent<String>::New(args[0]->ToString());
  Persistent<String> dst = Persistent<String>::New(args[2]->ToString());
  rotate_data->src_file = src;
  rotate_data->dst_file = dst;
  rotate_data->degrees  = degrees;
  
  Local<Function> cb = Local<Function>::Cast(args[3]);
  rotate_data->cb    = Persistent<Function>::New(cb);

  eio_custom(rotate, EIO_PRI_DEFAULT, rotate_callback, rotate_data);
  ev_ref(EV_DEFAULT_UC);

  return Undefined();
}

struct IdentifyData {
  Persistent<String>   src_file;
  Persistent<Function> cb;
};

int identify (eio_req *req) {

  IdentifyData *data = (IdentifyData*) req->data;

  String::Utf8Value src_path(data->src_file);

  IMAGE *img  = im_open(*src_path, "r");

  char* res = new char[32];
  im_meta_get_string(img, "Colorspace", &res);
  printf("result: %s\n", res);
  delete res;

  if( im_close(img) ) {
    printf("[error] failed to close image %s\n", *src_path);
  }

  return 0;
}

int identify_callback (eio_req *req) {

  HandleScope scope;

  ev_unref(EV_DEFAULT_UC);
  
  IdentifyData *data = (IdentifyData*)req->data;
  
  Local<Value> argv[2];
  argv[0] = Local<Value>::New(Null());

  TryCatch try_catch;

  data->cb->Call(Context::GetCurrent()->Global(), 2, argv);

  if (try_catch.HasCaught()) {
    FatalException(try_catch);
  }
  
  data->src_file.Dispose();
  data->cb.Dispose();
  
  delete data;
  
  return 0;
}


Handle<Value> Identify(const Arguments& args) {
  HandleScope scope;

  if (args.Length() < 2){
    return ThrowException(Exception::Error(
      String::New("Not enough parameters: identify(image_path, callback)")
    ));
  }
/*
  if (!Buffer::HasInstance(args[0])) {
    return ThrowException(Exception::Error(
      String::New("In identify(image_buffer), image_buffer must be a Buffer.")
    ));
  }
*/

  if (!args[0]->IsString()) {
    return ThrowException(Exception::Error(
      String::New("In identify(image_path, callback), image_path must be a String.")
    ));
  } else if (!args[1]->IsFunction()) {
    return ThrowException(Exception::Error(
      String::New("In identify(image_path, callback), callback must be a Function.")
    ));
  }

  IdentifyData *identify_data = new IdentifyData();

  Persistent<String> src = Persistent<String>::New(args[0]->ToString());
  identify_data->src_file = src;

  Local<Function> cb = Local<Function>::Cast(args[1]);
  identify_data->cb    = Persistent<Function>::New(cb);

  eio_custom(identify, EIO_PRI_DEFAULT, identify_callback, identify_data);
  ev_ref(EV_DEFAULT_UC);

  delete identify_data;

  return scope.Close(String::New("identify"));
}

}

extern "C" void init(Handle<Object> target) {
  HandleScope scope;

  NODE_SET_METHOD(target, "resize", Resize);
  NODE_SET_METHOD(target, "rotate", Rotate);
  NODE_SET_METHOD(target, "identify", Identify);
  
  g_type_init();
};


