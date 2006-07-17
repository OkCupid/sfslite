
$(PROGRAMS): $(LDEPS)

sfslib_LTLIBRARIES = libtame.la

libtame_la_SOURCES = pipeline.C

sfslib_HEADERS = pipeline.h

SUFFIXES = .C .T .h

.T.C:
	-$(TAME) -c $< || rm -f $@

pipeline.o: pipeline.C
pipeline.lo: pipeline.C

.PHONY: tameclean

tameclean:
	rm -f $(srcdir)/pipeline.C

EXTRA_DIST = .cvsignore pipeline.T
CLEANFILES = core *.core *~ *.rpo

MAINTAINERCLEANFILES = Makefile.in config.guess config.h.in config.sub \
	configure install-sh ltconfig ltmain.sh missing mkinstalldirs \
	stamp-h.in 

BUILT_SOURCES = pipeline.C
