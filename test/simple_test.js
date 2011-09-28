var testCase = require('nodeunit').testCase;
var fs = require('fs');
var vips = require('../node-vips');

module.exports = testCase({
  test_resize0: function(assert) {
    console.log("js: console.log(before vips.resize 0)");
    vips.resize("test/input.jpg", 960, 600, "test/output1.jpg", function(){
      console.log("js: console.log(after vips.resize 0)");
      assert.done();
    });
  },
/*
  test_resize1: function(assert) {
    fs.readFile('test/input.jpg', function(error, data) {
      if (error) {
        assert.ok(false, "failed to open input file");
        assert.done();
      }
      else {
        console.log("js: console.log(before vips.resize 1)");
        vips.resize(data, 200, 100, "test/output1.jpg", function(){
          console.log("js: console.log(after vips.resize 1)");
        });
      assert.done();
      }
    });
  },
  test_resize2: function(assert) {
    fs.readFile('test/input.jpg', function(error, data) {
      if (error) {
        assert.ok(false, "failed to open input file");
        assert.done();
      }
      else {
        console.log("js: console.log(before vips.resize 2)");
        vips.resize(data, 960, 600, "test/output2.jpg", function(){
          console.log("js: console.log(after vips.resize 2)");
        });
      assert.done();
      }
    });
  },
*/
  test_rotate0: function(assert) {
    console.log("js: console.log(before vips.rotate 0)");
    vips.rotate("test/input.jpg", 90, "test/output2.jpg", function(){
      console.log("js: console.log(after vips.rotate 0)");
      assert.done();
    });
  },
/*
  test_rotate1: function(assert) {
    fs.readFile('test/output1.jpg', function(error, data) {
      if (error) {
        assert.ok(false, "failed to open input file");
        assert.done();
      }
      else {
        vips.rotate(data, 90, "test/output2.jpg");
        assert.done();
      }
    });
  },
*/
  test_identity0: function(assert) {
    console.log("js: console.log(before vips.identity 0)");
    vips.identify("test/input.jpg", function(){
      console.log("js: console.log(after vips.identity 0)");
      assert.done();
    });
  },
/*
  test_identify1: function(assert) {
    fs.readFile('test/output2.jpg', function(error, data) {
      if (error) {
        assert.ok(false, "failed to open input file");
        assert.done();
      }
      else {
        var identity = vips.identify(data);
        console.log(identity);
        assert.done();
      }
    });
  },
*/
});

