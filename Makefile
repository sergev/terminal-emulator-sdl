#
# make
# make all      -- build everything
#
# make test     -- run unit tests
#
# make install  -- install binaries to /usr/local
#
# make clean    -- remove build files
#
# To reconfigure for Debug build:
#   make clean; make debug; make
#

all:    build
	$(MAKE) -Cbuild $@

test:   build
	$(MAKE) -Cbuild unit_tests
	ctest --test-dir build

install: build
	$(MAKE) -Cbuild $@

clean:
	rm -rf build

build:
	mkdir $@
	cmake -B$@ -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_PREFIX_PATH="/usr/local/opt/icu4c"

debug:
	mkdir build
	cmake -Bbuild -DCMAKE_BUILD_TYPE=Debug
