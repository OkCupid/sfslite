
$(PROGRAMS): $(LDEPS)

sfslib_LTLIBRARIES = libtame.la

sfsinclude_HEADERS = tame_pipeline.h tame_lock.h tame_autocb.h \
	tame.h tame_core.h tame_cancel.h tame_mkevent.h \
	tame_cfork.h tame_cfork_ag.h tame_thread.h

SUFFIXES = .C .T .h
.T.C:
	-$(TAME) -o $@ $< || rm -f $@

define(`tame_in')dnl
define(`tame_out')dnl

define(`tame_src',
changequote([[, ]])dnl
[[dnl
define(`tame_in', tame_in $1.T)dnl
define(`tame_out', tame_out $1.C)dnl
$1.o: $1.C
$1.lo: $1.C
]]changequote)dnl

dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl
dnl
dnl List all source files here
dnl

tame_src(pipeline)
tame_src(lock)

dnl
dnl
dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl

mkevent.o:  tame_mkevent.h mkevent.C
mkevent.lo: tame_mkevent.h mkevent.C

cfork.o:    tame_mkevent.h tame_cfork_ag.h cfork.C
cfork.lo:   tame_mkevent.h tame_cfork_ag.h cfork.C

tame_mkevent.h: $(srcdir)/mkevent.pl
	perl $< > $@

tame_cfork_ag.h: $(srcdir)/mkcfork_ag.pl
	perl $< > $@


libtame_la_SOURCES = mkevent.C cfork.C thread.C core.C tame_out 

.PHONY: tameclean

tameclean:
	rm -f tame_out

dist-hook:
	cd $(distdir) && rm -f tame_out

EXTRA_DIST = .svnignore tame_in Makefile.am.m4
CLEANFILES = core *.core *~ *.rpo tame_mkevent.h tame_out
MAINTAINERCLEANFILES = Makefile.in Makefile.am

$(srcdir)/Makefile.am: $(srcdir)/Makefile.am.m4
	@rm -f $(srcdir)/Makefile.am~
	$(M4) $(srcdir)/Makefile.am.m4 > $(srcdir)/Makefile.am~
	mv -f $(srcdir)/Makefile.am~ $(srcdir)/Makefile.am
