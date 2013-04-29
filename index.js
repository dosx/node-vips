try {
  module.exports = require('./build/Release/vips');
} catch(e) {
  //for < v0.5.5+
  module.exports = require('./build/default/vips');
}
