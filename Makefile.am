## Process this file with automake to produce Makefile.in
## $Id$

if USE_DB
ADB =
else
ADB = adb
endif

#
# sfslite change
#
SUBDIRS = async rpcc arpc crypt sfsmisc


noinst_HEADERS = nfsconf.h

EXTRA_DIST = setup autoconf.sed .cvsignore STANDARDS \
	INSTALL.darwin INSTALL.macos README.0.7-upgrade sfsauthd/upgradedb.pl

CLEANFILES = core *.core *~

MAINTAINERCLEANFILES = 				\
	aclocal.m4 install-sh mkinstalldirs missing depcomp \
	configure config.guess config.sub config.h.in \
	ltconfig ltmain.sh \
	INSTALL COPYING Makefile.in stamp-h.in


sfsinclude_HEADERS = autoconf.h
BUILT_SOURCES = autoconf.h
DISTCLEANFILES = autoconf.h stamp-auto-h autom4te*.cache
all-recursive install-recursive: autoconf.h

autoconf.h: stamp-auto-h
	@:

stamp-auto-h: $(srcdir)/autoconf.sed $(CONFIG_HEADER)
	@rm -f autoconf.h~
	sed -f $(srcdir)/autoconf.sed $(CONFIG_HEADER) > autoconf.h~
	cmp autoconf.h~ autoconf.h || mv -f autoconf.h~ autoconf.h
	@rm -f autoconf.h~
	@echo timestamp > stamp-auto-h

dist-hook:
	cd $(distdir) && rm -f autoconf.h
	cd $(distdir) && chmod -x setup

install-data-local:
	-src=$(DESTDIR)$(sfsincludedirp); dst=$(DESTDIR)$(pkgincludedir); \
	rm -f $$dst; \
	if test "`dirname $$src`" = "`dirname $$dst`"; then \
		ln -s "`basename $$src`" $$dst; \
	else \
		ln -s $(sfsincludedirp) $$dst; \
	fi

install-exec-local:
	-src=$(DESTDIR)$(sfslibdirp); dst=$(DESTDIR)$(pkglibdir); \
	rm -f $$dst; \
	if test "`dirname $$src`" = "`dirname $$dst`"; then \
		ln -s "`basename $$src`" $$dst; \
	else \
		ln -s $(sfslibdirp) $$dst; \
	fi

install-data-hook:
	@echo "********************** Upgrading SFS **************************"
	@echo If you previously installed older versions of SFS, you may want
	@echo to remove their directories and files:
	@echo
	@echo "   " $(prefix)/include/sfs-VERSION
	@echo "   " $(prefix)/lib/sfs-VERSION
	@echo
	@echo where VERSION is less than $(VERSION). Be certain to kill old 
	@echo versions of SFS before deleting their corresponding binaries.
	@echo "***************************************************************"

uninstall-local:
	-rm -f $(DESTDIR)$(pkglibdir) $(DESTDIR)$(pkgincludedir)
	-rmdir $(DESTDIR)$(sfslibdir) $(DESTDIR)$(sfsincludedir) \
		$(DESTDIR)$(confdir)

all-recursive-am: aux-all
#mostlyclean-recursive: aux-mostlyclean
#clean-recursive: aux-clean
distclean-recursive: aux-distclean
maintainer-clean-recursive: aux-maintainer-clean

AUX_TARGET = `echo $@ | sed -e 's/^aux-//'`
.PHONY: aux-all aux-mostlyclean aux-clean aux-distclean aux-maintainer-clean
aux-all aux-mostlyclean aux-clean aux-distclean aux-maintainer-clean:
	test -z "$(GMP_DIR)" || (cd "$(GMP_DIR)" && $(MAKE) $(AUX_TARGET))
	test -z "$(DB3_DIR)" -o "$(DB3_DIR)" = "@"DB3_DIR"@" \
		|| (cd "$(DB3_DIR)/dist" && $(MAKE) $(AUX_TARGET))

.PHONY: rpcclean
rpcclean:
	for dir in arpc svc libsfs; do \
		(cd $$dir && $(MAKE) rpcclean); \
	done
