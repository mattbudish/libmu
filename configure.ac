AC_INIT([libmu], [0.1.0])
AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_PROG_CC
AC_CONFIG_FILES([Makefile libmu.pc])
# Call libmicrohttpd's and libuv's ./configure scripts recursively.
AC_CONFIG_SUBDIRS([libmicrohttpd libuv])
AC_CONFIG_MACRO_DIRS([m4])
LT_INIT
AC_OUTPUT