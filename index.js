try {
  module.exports = require('./build/default/node-vips');
} catch(e) {
  //update for v0.5.5+
  module.exports = require('./build/Release/node-vips');
}
