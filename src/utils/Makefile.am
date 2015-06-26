#
# Process this file with automake to produce Makefile.in
#

AM_CPPFLAGS=-DDATADIRECTORY='"${pkgdatadir}"' -DLIBDIRECTORY='"${libdir}"' -DSYSCONFDIRECTORY='"${sysconfdir}/frepple"'

CLEANFILES = *.gcda *.gcov *.gcno

noinst_LTLIBRARIES = libutils.la
libutils_la_SOURCES = python.cpp date.cpp xml.cpp actions.cpp library.cpp name.cpp service.cpp

if LICENSE
# Full installation of all files
libutils_la_SOURCES += encrypt.cpp
else
# Distribute only secure components
nodist_libutils_la_SOURCES = encrypt.cpp
endif

