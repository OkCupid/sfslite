
sfsbin_PROGRAMS = rtftpc rtftpd

noinst_HEADERS = rtftp.h
rtftpc_SOURCES = rtftp_prot.C cli.C util.C
rtftpd_SOURCES = rtftp_prot.C srv.C util.C

if USE_SFSMISC
LIBSVC_ME = $(LIBSVC)
else
LIBSVC_ME =
endif

LDADD = $(LIBAAPP) $(LIBTAME) $(LIBSFSMISC) $(LIBSVC_ME) $(LIBSFSCRYPT) $(LIBARPC) $(LIBSAFEPTR) $(LIBASYNC) $(LIBGMP) $(LDADD_STD_ALL)

rtftpc_LDADD = $(LDADD)
rtftpd_LDADD = $(LDADD)

TAMEIN = cli.T srv.T
TAMEOUT = cli.C srv.C

SUFFIXES = .x .T
.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)

util.C: rtftp_prot.h
util.o: rtftp_prot.h
util.lo: rtftp_prot.h
rtftp_prot.C: rtftp_prot.h
rtftp_prot.lo: rtftp_prot.h rtftp_prot.C
rtftp_prot.o: rtftp_prot.h rtftp_prot.C
cli.o: rtftp_prot.h
srv.o: rtftp_prot.h
cli.lo: rtftp_prot.h
srv.lo: rtftp_prot.h


rtftp_prot.h: $(srcdir)/rtftp_prot.x
	@rm -f $@
	$(RPCC) -h $< || (rm -f $@ && false)

rtftp_prot.C: $(srcdir)/rtftp_prot.x
	@rm -f $@
	$(RPCC) -c $< || (rm -f $@ && false)


CLEANFILES = core *.core *~ *_config *_log $(TAMEOUT)
EXTRA_DIST = $(TAMEIN) rtftp_prot.x
MAINTAINERCLEANFILES = Makefile.in

.PHONY:	tameclean rpcclean

tameclean:
	rm -f $(TAMEOUT)

rpcclean:
	rm -f rtftp_prot.[Ch]

dist-hook:
	cd $(distdir) && rm -f $(TAMEOUT)
	cd $(distdir) && rm -f rtftp_prot.[Ch]

