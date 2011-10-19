// Copyright Erly Inc 2011, All Rights Reserved
// Authors: Bo Wang, Walt Lin
//
// Tests for node-vips.cc

var testCase = require('nodeunit').testCase;
var fs = require('fs');
var imagemagick = require('imagemagick');
var vips = require('../index');

var input1 = 'test/input.jpg';
var input2 = 'test/input2.jpg';

// Return a path for an output file.
var nextOutput = (function() {
  var id = 0;
  return function() { return 'test/output' + id++ + '.jpg'; }
})();

module.exports = testCase({
  test_resize_basic: function(assert) {
    vips.resize(input1, nextOutput(), 170, 170, true, false, function(err, m){
      assert.ok(!err, "unexpected error: " + err);
      assert.equals(170, m.width);
      assert.equals(170, m.height);
      assert.done();
    });
  },
  test_resize_basic_nocrop: function(assert) {
    vips.resize(input1, nextOutput(), 170, 170, false, false, function(err, m){
      assert.ok(!err, "unexpected error: " + err);
      assert.equals(170, m.width);
      assert.equals(128, m.height);
      assert.done();
    });
  },
  test_resize_with_auto_orient: function(assert) {
    vips.resize(input1, nextOutput(), 170, 170, true, true, function(err, m){
      assert.ok(!err, "unexpected error: " + err);
      assert.done();
    });
  },
  test_resize_with_auto_orient2: function(assert) {
    vips.resize(input2, nextOutput(), 170, 170, true, false, function(err, m){
      assert.ok(!err, "unexpected error: " + err);
      assert.done();
    });
  },
  test_resize_notfound: function(assert) {
    vips.resize("test/NOTFOUND", nextOutput(), 100, 100, true, true,
                function(err, metadata) {
      console.log("error: " + err);
      assert.ok(err, "expected error but did not get one");
      assert.done();
    });
  },
  test_resize_notfound_no_auto_orient: function(assert) {
    vips.resize("test/NOTFOUND", nextOutput(), 100, 100, true, false,
		function(err, metadata) {
      console.log("error: " + err);
      assert.ok(err, "expected error but did not get one");
      assert.done();
    });
  },
  test_resize_bogus_orientation_tag: function(assert) {
    vips.resize("test/bogus_value_for_orientation_tag.jpg", nextOutput(),
                100, 100, true, true, function(err, metadata) {
      assert.ok(!err, "unexpected error: " + err);
      assert.done();
    });
  },

  // Note: this test will crash if vips is compiled with imagemagick support
  // because imagemagick crashes when called from libeio.  If vips does not
  // have imagemagick support, this test will pass.
  test_resize_bad_input: function(assert) {
    vips.resize("test/bogus.jpg", nextOutput(), 100, 100, true, false,
		function(err, metadata) {
      assert.ok(err, "expected error but did not get one");
      assert.done();
    });
  },


  test_rotate_basic: function(assert) {
    var output = nextOutput();
    vips.rotate(input2, output, 90, function(err, metadata){
      assert.ok(!err, "unexpected error: " + err);

      // Ensure that the dimensions of the new rotated photo are correct.
      imagemagick.identify(output, function(err, metadata) {
        assert.ok(!err, "unexpected error: " + err);
        assert.equals(1920, metadata.height);
        assert.equals(1200, metadata.width);
        assert.done();
      });
    });
  },
  test_rotate_error_not_found: function(assert) {
    vips.rotate("test/NOTFOUND", nextOutput(), 90, function(err, metadata){
      assert.ok(err, "expected error but did not get one");
      assert.done();
    });
  },
  test_rotate_error_bad_degrees: function(assert) {
    vips.rotate(input1, nextOutput(), 93, function(err, metadata){
      assert.ok(err, "expected error but did not get one");
      assert.done();
    });
  },
});
