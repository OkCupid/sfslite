
$(PROGRAMS): $(LDEPS)

sfslib_LTLIBRARIES = libtame.la

SUFFIXES = .C .T .h .Th
.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)
.Th.h:
	$(TAME) -o $@ $< || (rm -f $@ && false)

define(`tame_in')dnl
define(`tame_out')dnl
define(`tame_out_h')dnl
define(`tame_in_h')dnl

define(`tame_src',
changequote([[, ]])dnl
[[dnl
define(`tame_in', tame_in $1.T)dnl
define(`tame_out', tame_out $1.C)dnl
$1.o: $1.C
$1.lo: $1.C
$1.C: $1.T $(TAME)
]]changequote)dnl

define(`tame_hdr',
changequote([[, ]])dnl
[[dnl
$1.h: $1.Th $(TAME)
define(`tame_out_h', tame_out_h $1.h)dnl
define(`tame_in_h', tame_in_h $1.Th)dnl
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
tame_src(pipeline3)
tame_src(connectors)
tame_hdr(tame_connectors)
tame_hdr(tame_nlock)
tame_hdr(tame_redundant)

dnl
dnl
dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl dnl

recycle.o:  tame_event_ag.h recycle.C
recycle.lo: tame_event_ag.h recycle.C

mkevent.o:  tame_event_ag.h mkevent.C
mkevent.lo: tame_event_ag.h mkevent.C

io.o: tame_connectors.h tame_nlock.h
io.lo: tame_connectors.h tame_nlock.h

tame_event_ag.h: $(srcdir)/mkevent.pl
	$(PERL) $(srcdir)/mkevent.pl > $@ || (rm -f $@ && false)

libtame_la_SOURCES = \
	recycle.C \
	closure.C \
	leak.C \
	init.C \
	run.C \
	mkevent.C \
	trigger.C \
	event.C \
	profiler.C \
	tame_out 

libtame_la_LDFLAGS = $(LIBTOOL_VERSION_INFO)

dnl
dnl All those headers before tame.h are tame internal headers; all after
dnl are tame libraries that are useful but not required.
dnl
sfsinclude_HEADERS = \
	tame_event.h \
	tame_run.h \
	tame_recycle.h \
	tame_weakref.h \
	tame_closure.h \
	tame_rendezvous.h \
	tame_event_ag.h \
	tame_typedefs.h \
	tame_slotset.h \
	tame.h \
	tame_pipeline.h \
	tame_lock.h \
	tame_autocb.h \
	tame_trigger.h \
	tame_pc.h \
	tame_io.h \
	tame_aio.h \
	tame_rpcserver.h \
	tame_rpc.h \
	tame_profiler.h \
	tame_pipeline3.h \
	tame_out_h 

.PHONY: tameclean

tameclean:
	rm -f tame_out tame_out_h

clean:
	rm -rf tame_out tame_out_h *.o *.lo .libs _libs core *.core *~ *.rpo \
		tame_event_ag.h *.la

dist-hook:
	cd $(distdir) && rm -f tame_out

EXTRA_DIST = .svnignore tame_in Makefile.am.m4 mkevent.pl  \
	tame_in_h
CLEANFILES = core *.core *~ *.rpo tame_event_ag.h tame_out \
	tame_out_h
MAINTAINERCLEANFILES = Makefile.in Makefile.am

$(srcdir)/Makefile.am: $(srcdir)/Makefile.am.m4
	@rm -f $(srcdir)/Makefile.am~
	$(M4) $(srcdir)/Makefile.am.m4 > $(srcdir)/Makefile.am~
	mv -f $(srcdir)/Makefile.am~ $(srcdir)/Makefile.am
