// Copyright Erly Inc 2011, All Rights Reserved
// Authors: Bo Wang, Walt Lin
//
// Node native extension to wrap the VIPS library for image manipulation.
//
// Javascript functions exported:
//
//   resize(input_path, output_path, new_x, new_y, crop_to_size,
//          auto_orient, callback<Error, metadata>)
//
//   rotate(input_path, output_path, degrees, callback<Error, metadata>)
//
// 'metadata' is an object with 'width' and 'height' properties.
// The functions will reject images they cannot open.

#include <node.h>
#include <stdio.h>
#include <string>

#include "transform.h"

using namespace v8;
using namespace node;

namespace {

// Macros for checking arguments.

#define REQ_FUN_ARG(I, VAR)                                             \
  if (args.Length() <= (I) || !args[I]->IsFunction())                   \
    return ThrowException(Exception::TypeError(		                \
                  String::New("Argument " #I " must be a function")));  \
  Local<Function> VAR = Local<Function>::Cast(args[I])
#define REQ_NUM_ARG(I, VAR)                                             \
  if (args.Length() <= (I) || !args[I]->IsNumber())                     \
    return ThrowException(Exception::TypeError(                         \
                  String::New("Argument " #I " must be a number")));    \
  int VAR = args[I]->Int32Value()
#define REQ_STR_ARG(I, VAR)                                             \
  if (args.Length() <= (I) || !args[I]->IsString())                     \
    return ThrowException(Exception::TypeError(                         \
                  String::New("Argument " #I " must be a string")));    \
  Local<String> VAR = args[I]->ToString()
#define REQ_BOOL_ARG(I, VAR)                                            \
  if (args.Length() <= (I) || !args[I]->IsBoolean())                    \
    return ThrowException(Exception::TypeError(                         \
                  String::New("Argument " #I " must be a boolean")));   \
  bool VAR = args[I]->BooleanValue()



// Data needed for a call to Transform.
// If cols or rows is <= 0, no resizing is done. 
// rotate_degrees must be one of 0, 90, 180, or 270.
struct TransformCall {
  int  cols;              // resize to this many columns
  int  rows;              // and this many rows
  bool crop_to_size;
  int  rotate_degrees;    // rotate image by this many degrees
  bool auto_orient;
  int  new_width;
  int  new_height;
  std::string src_path;
  std::string dst_path;
  std::string err_msg;
  Persistent<Function> cb;

  TransformCall() :
    cols(-1), rows(-1), crop_to_size(false), rotate_degrees(0),
    auto_orient(false), new_width(0), new_height(0) {}
};

int EIO_Transform(eio_req *req) {
  TransformCall* t = static_cast<TransformCall*>(req->data);
  return DoTransform(t->cols, t->rows, t->crop_to_size, t->rotate_degrees,
		     t->auto_orient, t->src_path, t->dst_path,
                     &t->new_width, &t->new_height, &t->err_msg);
}

// Done function that invokes a callback.
int TransformDone(eio_req *req) {
  HandleScope scope;
  TransformCall *c = static_cast<TransformCall*>(req->data);
  ev_unref(EV_DEFAULT_UC);
  
  Local<Value> argv[2];
  if (!c->err_msg.empty()) {  // req->result is NOT set correctly
    // Set up an error object.
    argv[0] = String::New(c->err_msg.data(), c->err_msg.size());
    argv[1] = Local<Value>::New(Null());
  } else {
    Local<Object> metadata = Object::New();
    metadata->Set(String::New("width"), Integer::New(c->new_width));
    metadata->Set(String::New("height"), Integer::New(c->new_height));
    argv[0] = Local<Value>::New(Null());
    argv[1] = metadata;
  }

  TryCatch try_catch;
  c->cb->Call(Context::GetCurrent()->Global(), 2, argv);
  if (try_catch.HasCaught()) {
    FatalException(try_catch);
  }
  
  c->cb.Dispose();  
  delete c;
  return 0;
}

// ResizeAsync(input_path, output_path, new_x, new_y, auto_orient, callback)
Handle<Value> ResizeAsync(const Arguments& args) {
  HandleScope scope;
  REQ_STR_ARG(0, input_path);
  REQ_STR_ARG(1, output_path);
  REQ_NUM_ARG(2, new_x_px);
  REQ_NUM_ARG(3, new_y_px);
  REQ_BOOL_ARG(4, crop_to_size);
  REQ_BOOL_ARG(5, auto_orient);
  REQ_FUN_ARG(6, cb);

  TransformCall *c = new TransformCall;
  c->cols = new_x_px;
  c->rows = new_y_px;
  c->crop_to_size = crop_to_size;
  c->auto_orient = auto_orient;
  c->src_path = *String::Utf8Value(input_path);
  c->dst_path = *String::Utf8Value(output_path);
  c->cb = Persistent<Function>::New(cb);

  eio_custom(EIO_Transform, EIO_PRI_DEFAULT, TransformDone, c);
  ev_ref(EV_DEFAULT_UC);
  return Undefined();
}

// RotateAsync(input_path, output_path, degrees, callback)
Handle<Value> RotateAsync(const Arguments& args) {
  HandleScope scope;
  REQ_STR_ARG(0, input_path);
  REQ_STR_ARG(1, output_path);
  REQ_NUM_ARG(2, degrees);
  REQ_FUN_ARG(3, cb);

  TransformCall *c = new TransformCall;
  c->rotate_degrees = degrees;
  c->src_path = *String::Utf8Value(input_path);
  c->dst_path = *String::Utf8Value(output_path);
  c->cb = Persistent<Function>::New(cb);
  
  eio_custom(EIO_Transform, EIO_PRI_DEFAULT, TransformDone, c);
  ev_ref(EV_DEFAULT_UC);
  return Undefined();
}

}  // anonymous namespace

extern "C" void init(Handle<Object> target) {
  InitTransform("node-vips.cc" /* don't have argv[0] */);
  HandleScope scope;
  NODE_SET_METHOD(target, "resize", ResizeAsync);
  NODE_SET_METHOD(target, "rotate", RotateAsync);
};
