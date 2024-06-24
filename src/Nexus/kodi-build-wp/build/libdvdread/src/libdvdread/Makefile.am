ACLOCAL_AMFLAGS = -Im4

AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src/dvdread $(CSS_CFLAGS)

dist_doc_DATA = AUTHORS ChangeLog COPYING NEWS README.md TODO

MAINTAINERCLEANFILES = ChangeLog

ChangeLog: $(wildcard $(srcdir)/.git/logs/HEAD)
	-cd $(srcdir) && git log > $(abs_builddir)/$(@)-tmp
	-test -s $(@)-tmp && mv $(@)-tmp $(@)
	-rm -f $(@)-tmp
	test -e $(@) || touch $(@)

EXTRA_DIST = msvc doc/footer.html doc/header.html

lib_LTLIBRARIES = libdvdread.la

libdvdread_la_SOURCES = \
	src/bitreader.c \
	src/bswap.h \
	src/dvd_input.c \
	src/dvd_input.h \
	src/dvd_reader.c \
	src/dvd_udf.c \
	src/dvdread_internal.h \
	src/ifo_print.c \
	src/ifo_read.c \
	src/logger.c \
	src/logger.h \
	src/md5.c \
	src/md5.h \
	src/nav_print.c \
	src/nav_read.c \
	msvc/contrib/win32_cs.h

libdvdread_la_LIBADD = $(CSS_LIBS)

libdvdread_la_LDFLAGS = -version-info $(DVDREAD_LTVERSION) \
	-export-symbols-regex "(^dvdread.*|^nav.*|^ifo.*|^DVD.*|^UDF.*)"

pkgincludedir = $(includedir)/dvdread
pkginclude_HEADERS = \
	src/dvdread/bitreader.h \
	src/dvdread/dvd_reader.h \
	src/dvdread/dvd_udf.h \
	src/dvdread/ifo_print.h \
	src/dvdread/ifo_read.h \
	src/dvdread/ifo_types.h \
	src/dvdread/nav_print.h \
	src/dvdread/nav_read.h \
	src/dvdread/nav_types.h \
	src/dvdread/version.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = misc/dvdread.pc

if APIDOC
all-local: apidoc
install-data-local: install-apidoc
uninstall-hook: uninstall-apidoc
endif

apidoc: stamp-doxygen

stamp-doxygen: doc/doxygen.cfg
	cd doc && doxygen doxygen.cfg
	touch $@

clean-local:
	-rm -Rf stamp-doxygen doc/html

install-apidoc:
	$(MKDIR_P) $(DESTDIR)$(htmldir)
	for file in doc/html/*; do \
		$(INSTALL_DATA) $$file "$(DESTDIR)$(htmldir)" || exit 1; \
	done;

uninstall-apidoc:
	-rm -Rf $(DESTDIR)$(htmldir)/*.png $(DESTDIR)$(htmldir)/*.html $(DESTDIR)$(htmldir)/*.css $(DESTDIR)$(htmldir)/*.js
