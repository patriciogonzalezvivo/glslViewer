SUBDIRS = src
if COMPILE_EXAMPLES
SUBDIRS += examples
endif
SUBDIRS += lo build cmake @DOXYGEN@

EXTRA_DIST = libtool ltmain.sh autogen.sh

ACLOCAL_AMFLAGS = -I m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = liblo.pc

test: all
	(cd src && make test)
