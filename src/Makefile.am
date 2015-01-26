CC = gcc -std=gnu99
DFLAGS = #-DVERBOSE
DBGFLAGS = -Wall -Werror -O0 -ggdb
CFLAGS = -Wall -Werror -O0 -ggdb $(DFLAGS) $(DBGFLAGS) -I/usr/include/libxml2 `pkg-config --cflags glib-2.0`
AM_LDFLAGS = -lpth -lxml2 `pkg-config --libs glib-2.0`

#LIBNAME=@LIB_IDENTIFIER@
#lib_LTLIBRARIES=lib$(LIBNAME).la
#lib$(LIBNAME)_la_SOURCES = libpdsns.c
#lib$(LIBNAME)_la_LDFLAGS = -version-info PACKAGE_VERSION

lib_LTLIBRARIES=libpdsns.la
libpdsns_la_SOURCES = libpdsns.c
libpdsns_la_LDFLAGS = # -version-info PACKAGE_VERSION # tohle se z nejakyho duvodu zlobi

include_HEADERS = libpdsns.h

#bin_PROGRAMS = $(top_builddir)/bin/@PROGRAM_IDENTIFIER@
#__top_builddir__bin_@PROGRAM_IDENTIFIER@_SOURCES = main.c common.c cfg.c

clean-local:
	$(RM) -r $(top_builddir)/bin
