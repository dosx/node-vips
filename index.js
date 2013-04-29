try {
  module.exports = require('./build/default/vips');
} catch(e) {
  //update for v0.5.5+
  module.exports = require('./build/Release/vips');
}
