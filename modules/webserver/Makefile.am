#
# Process this file with automake to produce Makefile.in md5.inl
#

EXTRA_DIST = CivetServer.h civetweb.h webserver.h md5.inl json.h

CLEANFILES = *.gcda *.gcov *.gcno

pkglib_LTLIBRARIES = mod_webserver.la
mod_webserver_la_LDFLAGS = -module -shrext .so -avoid-version
mod_webserver_la_SOURCES = CivetServer.cpp civetweb.c module.cpp webserver.cpp webclient.cpp http_get.cpp websocket_get.cpp websocket_chat.cpp json.cpp
mod_webserver_la_LIBADD = ../../src/libfrepple.la
mod_webserver_la_CPPFLAGS = -DUSE_IPV6 -DUSE_WEBSOCKET

if CYGWIN
# Cygwin doesn't like undefined symbols in a windows dll
mod_webserver_la_LDFLAGS += -no-undefined
endif

# A convenience copy to quickly collect the results
all-local:
	-cp .libs/mod_webserver.so ../../bin
