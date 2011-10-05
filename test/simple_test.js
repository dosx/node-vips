// Copyright Erly Inc 2011, All Rights Reserved
// Authors: Bo Wang, Walt Lin
//
// Tests for node-vips.cc

var testCase = require('nodeunit').testCase;
var fs = require('fs');
var vips = require('../node-vips');

var input1 = 'test/input.jpg';
var input2 = 'test/input2.jpg';

// Return a path for an output file.
var nextOutput = (function() {
  var id = 0;
  return function() { return 'test/output' + id++ + '.jpg'; }
})();

module.exports = testCase({
  test_resize_basic: function(assert) {
    vips.resize(input1, nextOutput(), 227, 170, false, function(err){
      console.log('in resize callback js');
      assert.ok(!err, "unexpected error: " + err);
      assert.done();
    });
  },
  test_resize_with_auto_orient: function(assert) {
    vips.resize(input1, nextOutput(), 170, 170, true, function(err){
      console.log('in resize callback js');
      assert.ok(!err, "unexpected error: " + err);
      assert.done();
    });
  },
  test_resize_with_auto_orient2: function(assert) {
    vips.resize(input2, nextOutput(), 227, 170, false, function(err){
      console.log('in resize callback js');
      assert.ok(!err, "unexpected error: " + err);
      assert.done();
    });
  },
  test_resize_notfound: function(assert) {
    vips.resize("test/NOTFOUND", nextOutput(), 100, 100, true, function(err){
      console.log("error: " + err);
      assert.ok(err, "expected error but did not get one");
      assert.done();
    });
  },
  test_resize_notfound_no_auto_orient: function(assert) {
    vips.resize("test/NOTFOUND", nextOutput(), 100, 100, false, function(err){
      console.log("error: " + err);
      assert.ok(err, "expected error but did not get one");
      assert.done();
    });
  },

  // Note: this test will crash if vips is compiled with imagemagick support
  // because imagemagick crashes when called from libeio.  If vips does not
  // have imagemagick support, this test will pass.
  test_resize_bad_input: function(assert) {
    vips.resize("test/bogus.jpg", nextOutput(), 100, 100, false, function(err){
      assert.ok(err, "expected error but did not get one");
      assert.done();
    });
  },

  test_rotate_basic: function(assert) {
    console.log("js: console.log(before vips.rotate 0)");
    vips.rotate(input1, nextOutput(), 90, function(err){
      assert.ok(!err, "unexpected error: " + err);
      assert.done();
    });
  },
  test_rotate_error_not_found: function(assert) {
    vips.rotate("test/NOTFOUND", nextOutput(), 90, function(err){
      assert.ok(err, "expected error but did not get one");
      assert.done();
    });
  },
  test_rotate_error_bad_degrees: function(assert) {
    vips.rotate(input1, nextOutput(), 93, function(err){
      assert.ok(err, "expected error but did not get one");
      assert.done();
    });
  },
});
