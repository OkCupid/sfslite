
$(PROGRAMS): $(LDEPS)

sfslib_LTLIBRARIES = libtame.la

sfsinclude_HEADERS = tame_pipeline.h tame_lock.h tame_autocb.h \
	tame.h tame_core.h tame_event_ag.h \
	tame_tfork.h tame_tfork_ag.h tame_thread.h tame_trigger.h \
	tame_pc.h tame_io.h tame_event.h tame_recycle.h \
	tame_typedefs.h tame_connectors.h tame_aio.h \
	tame_nlock.h tame_rpcserver.h

SUFFIXES = .C .T .h .Th
.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)
.Th.h:
	$(TAME) -o $@ $< || (rm -f $@ && false)

define(`tame_in')dnl
define(`tame_out')dnl
define(`tame_out_h')dnl

define(`tame_src',
changequote([[, ]])dnl
[[dnl
define(`tame_in', tame_in $1.T)dnl
define(`tame_out', tame_out $1.C)dnl
$1.o: $1.C
$1.lo: $1.C
]]changequote)dnl

define(`tame_hdr',
changequote([[, ]])dnl
[[dnl
$1.h: $1.Th
define(`tame_out_h', tame_out_h $1.h)dnl
]]changequote)dnl


dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl
dnl
dnl List all source files here
dnl

tame_src(pipeline)
tame_src(lock)
tame_src(io)
tame_src(aio)
tame_src(rpcserver)
tame_hdr(tame_connectors)
tame_hdr(tame_nlock)

dnl
dnl
dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl

mkevent.o:  tame_event_ag.h mkevent.C
mkevent.lo: tame_event_ag.h mkevent.C

tfork.o:    tame_event_ag.h tame_tfork_ag.h tfork.C
tfork.lo:   tame_event_ag.h tame_tfork_ag.h tfork.C

io.o: tame_connectors.h
io.lo: tame_connectors.h

tame_event_ag.h: $(srcdir)/mkevent.pl
	perl $< > $@

tame_tfork_ag.h: $(srcdir)/mktfork_ag.pl
	perl $< > $@


libtame_la_SOURCES = mkevent.C tfork.C thread.C core.C trigger.C event.C \
	recycle.C tame_out 

.PHONY: tameclean

tameclean:
	rm -f tame_out tame_out_h

clean:
	rm -f tame_out tame_out_h

dist-hook:
	cd $(distdir) && rm -f tame_out

EXTRA_DIST = .svnignore tame_in Makefile.am.m4 mkevent.pl mktfork_ag.pl
CLEANFILES = core *.core *~ *.rpo tame_event_ag.h tame_tfork_ag.h tame_out
MAINTAINERCLEANFILES = Makefile.in Makefile.am

$(srcdir)/Makefile.am: $(srcdir)/Makefile.am.m4
	@rm -f $(srcdir)/Makefile.am~
	$(M4) $(srcdir)/Makefile.am.m4 > $(srcdir)/Makefile.am~
	mv -f $(srcdir)/Makefile.am~ $(srcdir)/Makefile.am
