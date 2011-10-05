// Copyright Erly Inc 2011, All Rights Reserved
// Authors: Bo Wang, Walt Lin
//
// Node native extension to wrap the VIPS library for image manipulation.
//
// Javascript functions exported:
//   resize(input_path, output_path, new_x, new_y, auto_orient, callback)
//   rotate(input_path, output_path, degrees, callback)
// output_path can have an optional ":jpeg_quality" appended to it.
//
// Both callbacks take just an Error object.  The functions will reject
// images they cannot open.

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
  int cols;              // resize to this many columns
  int rows;              // and this many rows
  int rotate_degrees;    // rotate image by this many degrees
  bool auto_orient;
  std::string src_path;
  std::string dst_path;
  std::string err_msg;
  Persistent<Function> cb;

  TransformCall() : cols(-1), rows(-1), rotate_degrees(0), auto_orient(false) {}
};

int EIO_Transform(eio_req *req) {
  TransformCall* t = static_cast<TransformCall*>(req->data);
  return DoTransform(t->cols, t->rows, t->rotate_degrees, t->auto_orient,
		     t->src_path, t->dst_path, &t->err_msg);
}

// Done function that invokes a callback with one arg: an Error (or NULL).
int TransformDone(eio_req *req) {
  HandleScope scope;
  TransformCall *c = static_cast<TransformCall*>(req->data);
  ev_unref(EV_DEFAULT_UC);
  
  Local<Value> argv[1];
  if (!c->err_msg.empty()) {  // req->result is NOT set correctly
    // Set up an error object.
    argv[0] = String::New(c->err_msg.data(), c->err_msg.size());
  } else {
    argv[0] = Local<Value>::New(Null());
  }

  TryCatch try_catch;
  c->cb->Call(Context::GetCurrent()->Global(), 1, argv);
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
  REQ_BOOL_ARG(4, auto_orient);
  REQ_FUN_ARG(5, cb);

  TransformCall *c = new TransformCall;
  c->cols = new_x_px;
  c->rows = new_y_px;
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
  HandleScope scope;
  NODE_SET_METHOD(target, "resize", ResizeAsync);
  NODE_SET_METHOD(target, "rotate", RotateAsync);
};
