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
ifneq ($(wildcard /opt/homebrew/opt/icu4c/.),)
    CMAKE_OPTIONS := -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/icu4c
endif
ifneq ($(wildcard /usr/local/opt/icu4c/.),)
    CMAKE_OPTIONS := -DCMAKE_PREFIX_PATH=/usr/local/opt/icu4c
endif

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
	cmake -B$@ -DCMAKE_BUILD_TYPE=RelWithDebInfo $(CMAKE_OPTIONS)

debug:
	mkdir build
	cmake -Bbuild -DCMAKE_BUILD_TYPE=Debug
