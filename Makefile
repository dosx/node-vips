TESTS = test/*.js

all: test

build: clean configure compile

configure:
	PKG_CONFIG_PATH=/usr/local/Library/Homebrew/pkgconfig node-waf configure

compile:
	node-waf build

test: build
	@./node_modules/nodeunit/bin/nodeunit \
		$(TESTS)

clean:
	rm -f node-vips.node test/output*.jpg
	rm -Rf build


.PHONY: clean test build
