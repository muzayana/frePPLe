#
# Process this file with automake to produce Makefile.in
#

AM_CPPFLAGS=-DDATADIRECTORY='"@datadir@/frepple"' -DLIBDIRECTORY='"@libdir@"'

CLEANFILES = *.gcda *.gcov *.gcno

noinst_LTLIBRARIES = libutils.la
libutils_la_SOURCES = pythonutils.cpp date.cpp xmlparser.cpp actions.cpp library.cpp name.cpp

if LICENSE
# Full installation of all files
libutils_la_SOURCES += encrypt.cpp
else
# Distribute only secure components
nodist_libutils_la_SOURCES = encrypt.cpp
endif

