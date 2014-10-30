## Process this file with automake to produce Makefile.in
# $Id$

sfslib_LTLIBRARIES = libasync.la
LIBASYNC = libasync.la
sfsexec_PROGRAMS = aiod mmcd

libasync_la_SOURCES = \
arandom.c clock_gettime.c flock.c rwfd.c stktrace.c suidprotect.c \
aerr.C aio.C aios.C arena.C armor.C bbuddy.C cbuf.C convertint.C	\
core.C daemonize.C dns.C dnsparse.C err.C fdwait.C ident.C ifchg.C	\
ihash.C itree.C lockfile.C malloc.C msb.C myaddrs.C myname.C		\
parseopt.C pipe2str.C refcnt.C rxx.C sigio.C socket.C spawn.C str.C	\
str2file.C straux.C suio++.C suio_vuprintf.C tcpconnect.C litetime.C \
select.C select_std.C select_epoll.C select_kqueue.C dynenum.C \
vec.C bundle.C alog2.C leakcheck.C profiler.C wide_str.C const.C

libasync_la_LDFLAGS = $(LIBTOOL_VERSION_INFO)

aiod_SOURCES = aiod.C
aiod_LDADD = $(LIBASYNC) $(LIBPY) $(LDADD_THR) $(LDADD_STD_ALL)
aiod_DEPENDENCIES = $(LIBASYNC)

sfsinclude_HEADERS = \
aiod.h aiod_prot.h aios.h amisc.h arena.h array.h async.h backoff.h	\
bbuddy.h bitvec.h callback.h cbuf.h dns.h dnsimpl.h dnsparse.h err.h	\
fdlim.h ihash.h init.h itree.h keyfunc.h list.h msb.h opnew.h		\
parseopt.h qhash.h refcnt.h rxx.h serial.h stllike.h str.h	\
suio++.h sysconf.h union.h vatmpl.h vec.h rwfd.h litetime.h       	\
corebench.h qtailq.h sfs_select.h rclist.h dynenum.h         \
rctailq.h rctree.h sfs_bundle.h alog2.h sfs_profiler.h wide_str.h 	\
sfs_const.h sfs_assert.h weak_template.h

#
# begin sfslite changes
#
mmcd_SOURCES = mmcd.c
mmcd_LDADD = 
mmcd_DEPENDENCIES = 


#
# end sfslite changes
#

noinst_HEADERS = pyenv.mk env.mk

DEPEND_ON_MAKEFILE = daemonize.o daemonize.lo spawn.o spawn.lo
$(DEPEND_ON_MAKEFILE): Makefile

if REPO
async_repo_OBJECTS = $(libasync_la_OBJECTS)
stamp-async-repo: $(async_repo_OBJECTS)
	-$(CXXLINK) $(async_repo_OBJECTS)
	@rm -f a.out
	touch $@
libasync_la_DEPENDENCIES = stamp-async-repo
endif

stktrace.lo: $(srcdir)/stktrace.c $(srcdir)/sysconf.h
	$(LTCOMPILE) $(NW) -c $(srcdir)/stktrace.c

#
# include Makefile flags and options for pysfs1 
#
pyenv.mk: Makefile
	@rm -f $@~
	echo "# Makefile fragment generated " `date` > $@~
	echo "CC = $(CC)" >> $@~
	echo "CXX = $(CXX)" >> $@~
	echo "CFLAGS = $(CFLAGS)" >> $@~ 
	echo "CXXFLAGS = $(CXXFLAGS)" >> $@~
	echo "LIBS = $(LIBS)" >> $@~
	echo "LIBGMP = $(LIBGMP)" >> $@~
	echo "LDFLAGS = $(LDFLAGS)" >> $@~
	mv -f $@~ $@

#
# for OKWS and other libraries built from this SFS library,
# need to know build options for profiling / etc.
#
env.mk: Makefile
	@rm -f $@~
	echo "# Makefile fragment generated " `date` > $@~
	echo "LDADD_THR = $(LDADD_THR)" >> $@~
	echo "LDADD_STD_ALL = $(LDADD_STD_ALL)" >> $@~	
	echo "CFLAGS_PROFILE = $(CFLAGS_PROFILE)" >> $@~
	echo "SFSLITE_ABI_VERSION = $(ABI_VERSION)" >> $@~
	mv -f $@~ $@

EXTRA_DIST = .cvsignore pyenv.mk env.mk
CLEANFILES = core *.core *~ *.rpo \
	stamp-async-repo env.mk env2.mk
MAINTAINERCLEANFILES = Makefile.in pyenv.mk env.mk

.PHONY: cbh
cbh:
	cd $(srcdir) && sh ./callback.h > callback.h~ \
		     && mv -f callback.h~ callback.h 

.PHONY: vah
vah:
	cd $(srcdir) && sh ./vatmpl.h > vatmpl.h~ \
		&& mv -f vatmpl.h~ vatmpl.h
