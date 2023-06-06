ACLOCAL_AMFLAGS = -I m4
# Build the libraries in the libmicrohttpd and libuv subdirectories first.
SUBDIRS = libmicrohttpd libuv

# Include libmicrohttpd's and libuv's headers when compiling this directory.
AM_CPPFLAGS = -I$(srcdir)/libmicrohttpd/src/include \
              -I$(srcdir)/libuv/include \
              -I$(srcdir)/include

include_HEADERS = include/mu.h

lib_LTLIBRARIES = libmu.la
libmu_la_SOURCES = src/mu.c

EXTRA_DIST = \
  libmu.pc.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libmu.pc