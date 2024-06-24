ACLOCAL_AMFLAGS = -Im4

AM_CPPFLAGS = -I$(top_srcdir)/src \
              -I$(top_builddir)/src/dvdnav \
              $(THREAD_CFLAGS) \
              $(DVDREAD_CFLAGS)

dist_doc_DATA = AUTHORS ChangeLog COPYING README.md TODO

EXTRA_DIST = msvc/ \
	     doc/doxy.conf \
	     doc/mainpage.cpp \
	     doc/tutorial.cpp \
	     doc/dvd_structures \
	     doc/library_layout

lib_LTLIBRARIES = libdvdnav.la

libdvdnav_la_SOURCES = \
	src/dvdnav.c \
	src/read_cache.c \
	src/navigation.c \
	src/highlight.c \
        src/logger.c \
        src/logger.h \
	src/searching.c \
	src/settings.c \
	src/dvdnav_internal.h \
	src/read_cache.h \
	src/vm/decoder.c \
	src/vm/decoder.h \
	src/vm/vm.c \
	src/vm/vm.h \
	src/vm/play.c \
	src/vm/play.h \
	src/vm/getset.c \
	src/vm/getset.h \
	src/vm/vmget.c \
	src/vm/vmcmd.c \
	src/vm/vmcmd.h

libdvdnav_la_LDFLAGS = \
	-version-info $(DVDNAV_LT_CURRENT):$(DVDNAV_LT_REVISION):$(DVDNAV_LT_AGE) \
	-export-symbols-regex "^dvdnav"
libdvdnav_la_LIBADD = $(THREAD_LIBS) $(DVDREAD_LIBS)

pkgincludedir = $(includedir)/dvdnav

pkginclude_HEADERS = \
	src/dvdnav/dvdnav.h \
	src/dvdnav/dvdnav_events.h \
	src/dvdnav/dvd_types.h \
	src/dvdnav/version.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = misc/dvdnav.pc

EXTRA_PROGRAMS = menus

menus_SOURCES = examples/menus.c
menus_LDADD = libdvdnav.la

docs: misc/doxy.conf
	@if test x"$(DOXYGEN)" != "xno"; then \
	  cd misc && $(DOXYGEN) doxy.conf > /dev/null 2> /dev/null; \
	fi
