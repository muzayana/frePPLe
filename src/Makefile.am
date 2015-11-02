#
# Process this file with automake to produce Makefile.in
#

bin_PROGRAMS = frepple
lib_LTLIBRARIES = libfrepple.la libfrepplepyd.la

# License creation utilities...
if LICENSE
# Build all components, including hidden ones
bin_PROGRAMS += createkey createlicense
createkey_SOURCES = createkey.cpp
createlicense_SOURCES = createlicense.cpp
createkey_LDADD = libfrepple.la
createlicense_LDADD = libfrepple.la
createkey_LDFLAGS = -dynamic -export-dynamic
createlicense_LDFLAGS = -dynamic -export-dynamic
endif

# The local target builds the main executables first, and then copies the
# output to the bin directory
all-local: frepple$(EXEEXT)
	if test -f .libs/frepple$(EXEEXT); then cp .libs/frepple$(EXEEXT) ../bin; fi
	if test -f .libs/cygfrepple*.dll; then cp .libs/cygfrepple*.dll ../bin; fi
	if test -f .libs/libfrepple.so; then cp .libs/libfrepple*.so* ../bin; fi
	if test -f .libs/createlicense$(EXEEXT); then cp .libs/createlicense$(EXEEXT) ../bin; fi
	if test -f .libs/createkey$(EXEEXT); then cp .libs/createkey$(EXEEXT) ../bin; fi

# Shared library
libfrepple_la_SOURCES = dllmain.cpp tags.cpp
libfrepple_la_LDFLAGS = -version-info @LIBRARY_VERSION@ -no-undefined
if CYGWIN
# Cygwin doesn't like undefined symbols in a windows dll
libfrepple_la_LDFLAGS += -Wl,--export-all-symbols
endif
libfrepple_la_LIBADD = ./solver/libsolver.la ./model/libmodel.la ./utils/libutils.la

# Python extension
libfrepplepyd_la_SOURCES = pythonextension.cpp
libfrepplepyd_la_LDFLAGS = -version-info @LIBRARY_VERSION@ -no-undefined -dynamic -export-dynamic
if CYGWIN
# Cygwin doesn't like undefined symbols in a windows dll
libfrepplepyd_la_LDFLAGS += -Wl,--export-all-symbols
endif
libfrepplepyd_la_LIBADD = libfrepple.la

# Dynamically linked executable
frepple_SOURCES = main.cpp
frepple_LDFLAGS = -dynamic -export-dynamic
frepple_LDADD = libfrepple.la

SUBDIRS = utils model solver

EXTRA_DIST = dll.rc exe.rc frepple.ico

CLEANFILES = *.gcda *.gcov *.gcno
