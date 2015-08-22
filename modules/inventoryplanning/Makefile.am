#
# Process this file with automake to produce Makefile.in
#

sources = inventoryplanning.cpp inventoryplanning.h

EXTRA_DIST = mod_inventoryplanning.xsd $(sources)

CLEANFILES = *.gcda *.gcov *.gcno

pkglib_LTLIBRARIES = mod_inventoryplanning.la
mod_inventoryplanningdir = $(datadir)/frepple
mod_inventoryplanning_DATA = mod_inventoryplanning.xsd
mod_inventoryplanning_la_LDFLAGS = -module -shrext .so -avoid-version
mod_inventoryplanning_la_SOURCES = $(sources)
mod_inventoryplanning_la_LIBADD = ../../src/libfrepple.la

if CYGWIN
# Cygwin doesn't like undefined symbols in a windows dll
mod_inventoryplanning_la_LDFLAGS += -no-undefined
endif

# A convenience copy to quickly collect the results
all-local:
	-cp .libs/mod_inventoryplanning.so ../../bin
	cp mod_inventoryplanning.xsd ../../bin
