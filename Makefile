TESTS = test/*.js

all: test

build: configure compile

configure:
	node-gyp configure

compile:
	node-gyp build

node_modules/.bin/nodeunit:
	# Installing nodeunit under --dev is not good...
	npm install nodeunit
	npm install --dev

test: build node_modules/.bin/nodeunit
	@./node_modules/.bin/nodeunit \
		$(TESTS)

clean:
	rm -f node-vips.node test/output*.jpg
	rm -rf build node_modules


.PHONY: clean test build compile all configure
