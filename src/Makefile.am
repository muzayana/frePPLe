#
# Process this file with automake to produce Makefile.in
#

bin_PROGRAMS = frepple createkey createlicense

# The local target builds the main executables first, and then copies the
# output to the bin directory
all-local:
	if test -f .libs/frepple$(EXEEXT); then cp .libs/frepple$(EXEEXT) ../bin; fi
	if test -f .libs/cygfrepple*.dll; then cp .libs/cygfrepple*.dll ../bin; fi
	if test -f .libs/libfrepple.so; then cp .libs/libfrepple*.so* ../bin; fi
	if test -f .libs/createlicense$(EXEEXT); then cp .libs/createlicense$(EXEEXT) ../bin; fi
	if test -f .libs/createkey$(EXEEXT); then cp .libs/createkey$(EXEEXT) ../bin; fi

# Shared library
pkglib_LTLIBRARIES = libfrepple.la
libfrepple_la_SOURCES = dllmain.cpp tags.cpp
libfrepple_la_LDFLAGS = -version-info @LIBRARY_VERSION@ -no-undefined
if CYGWIN
# Cygwin doesn't like undefined symbols in a windows dll
libfrepple_la_LDFLAGS += -Wl,--export-all-symbols
endif
libfrepple_la_LIBADD = ./solver/libsolver.la ./model/libmodel.la ./utils/libutils.la

# Dynamically linked executables
frepple_SOURCES = main.cpp
frepple_LDFLAGS = -dynamic -export-dynamic
frepple_LDADD = libfrepple.la
# Note that we do NOT want to distribute the source code of the some source files!!!
# These contain the private key we use to generate license keys.
nodist_createkey_SOURCES = createkey.cpp
nodist_createlicense_SOURCES = createlicense.cpp
createkey_LDADD = libfrepple.la
createlicense_LDADD = libfrepple.la
createkey_LDFLAGS = -dynamic -export-dynamic
createlicense_LDFLAGS = -dynamic -export-dynamic

SUBDIRS = utils model solver

EXTRA_DIST = dll.rc exe.rc

CLEANFILES = *.gcda *.gcov *.gcno
