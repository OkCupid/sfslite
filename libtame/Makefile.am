
$(PROGRAMS): $(LDEPS)

sfslib_LTLIBRARIES = libtame.la

libtame_la_SOURCES = pipeline.C

sfslib_HEADERS = pipeline.h

SUFFIXES = .C .T .h

.T.C:
	-$(TAME) -c $< || rm -f $@

pipeline.h: $(srcdir)/pipeline.Th
	-$(TAME) -o $(srcdir)/pipeline.h $< || rm -f $@

pipeline.o: pipeline.C
pipeline.lo: pipeline.C
pipeline.C: pipeline.h

.PHONY: tameclean

tameclean:
	rm -f $(srcdir)/pipeline.C $(srcdir)/pipeline.h

EXTRA_DIST = .cvsignore ok.T xmlrpcsrv.Th xmlrpcsrv.C
CLEANFILES = core *.core *~ *.rpo

MAINTAINERCLEANFILES = Makefile.in config.guess config.h.in config.sub \
	configure install-sh ltconfig ltmain.sh missing mkinstalldirs \
	stamp-h.in 

BUILT_SOURCES = pipeline.h pipeline.C
