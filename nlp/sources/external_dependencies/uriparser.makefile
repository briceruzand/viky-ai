#
# Extrenal lib uriparser
#

CURRENT_MAKEFILE := $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))

include $(OG_REPO_PATH)/sources/makefile.defs.linux

.PHONY: all redebug debug build make clean fullclean uriparser

debug: $(DBINPATH)/liburiparser.so $(SRCPATH)/include/uriparser/Uri.h

build: $(RBINPATH)/liburiparser.so $(SRCPATH)/include/uriparser/Uri.h

rebuild:
	$(MAKE) -f $(CURRENT_MAKEFILE) clean
	$(MAKE) -f $(CURRENT_MAKEFILE) build

redebug:
	$(MAKE) -f $(CURRENT_MAKEFILE) clean
	$(MAKE) -f $(CURRENT_MAKEFILE) debug


uriparser: all

all:
	$(MAKE) -f $(CURRENT_MAKEFILE) fullclean
	$(MAKE) -f $(CURRENT_MAKEFILE) debug build

fullclean: clean
	-cd uriparser && $(MAKE) clean
	-cd uriparser && git clean -dfx
	rm -rf uriparser/Makefile

clean:
	rm -f $(SRCPATH)/include/uriparser/*.h
	rm -f $(DBINPATH)/liburiparser.so*
	rm -f $(RBINPATH)/liburiparser.so*

make: uriparser/Makefile
	cd uriparser && $(MAKE)

uriparser/Makefile:
	cd uriparser && cmake -DCMAKE_BUILD_TYPE=Release \
												-DCMAKE_INSTALL_PREFIX:PATH="$(SRCPATH)/include" \
												-DCMAKE_INCLUDE_PATH:PATH="$(DBINPATH)" \
												-DCMAKE_LIBRARY_PATH:PATH="$(DBINPATH)" \
												-DURIPARSER_BUILD_DOCS:BOOL=OFF \
												-DURIPARSER_BUILD_TESTS:BOOL=OFF \
												-DURIPARSER_BUILD_TOOLS:BOOL=OFF \
												-DURIPARSER_BUILD_WCHAR_T:BOOL=OFF

$(DBINPATH)/liburiparser.so: make
	mkdir -p $(DBINPATH)
	cp -af uriparser/liburiparser.so* $(DBINPATH)/

$(RBINPATH)/liburiparser.so: $(DBINPATH)/liburiparser.so
	mkdir -p $(RBINPATH)
	cp -af uriparser/liburiparser.so* $(RBINPATH)/

$(SRCPATH)/include/uriparser/Uri.h: make
	mkdir -p $(SRCPATH)/include/uriparser
	cp -af uriparser/include/uriparser/*.h $(SRCPATH)/include/uriparser


