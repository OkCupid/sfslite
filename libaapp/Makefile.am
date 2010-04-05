
sfslib_LTLIBRARIES = libaapp.la

libaapp_la_SOURCES = aapp_prot.C accept.C logger.C
sfsinclude_HEADERS = aapp_prot.h aapp.h sfs_logger.h

libaapp_la_LDFLAGS = $(LIBTOOL_VERSION_INFO)

aapp_prot.o: aapp_prot.h
aapp_prot.lo: aapp_prot.h
accept.o: aapp_prot.h
accept.lo: aapp_prot.h

SUFFIXES = .C .T .h .Th
.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)
.Th.h:
	$(TAME) -o $@ $< || (rm -f $@ && false)

aapp_prot.h: $(srcdir)/aapp_prot.x
	@rm -f $@
	$(RPCC) -h $< || (rm -f $@ && false)

aapp_prot.C: $(srcdir)/aapp_prot.x
	@rm -f $@
	$(RPCC) -c $< || (rm -f $@ && false)

.PHONY: rpcclean
rpcclean:
	rm -f aapp_prot.C aapp_prot.h

EXTRA_DIST = aapp_prot.x 
CLEANFILES = core *.core *~ *.rpo aapp_prot.h aapp_prot.C
MAINTAINERCLEANFILES = Makefile.in

