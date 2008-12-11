
sfsbin_PROGRAMS = tinetd

tinetd_SOURCES = tinetd.C log.C

if USE_SFSMISC
LIBSVC_ME = $(LIBSVC)
else
LIBSVC_ME =
endif

LDADD = $(LIBAAPP) $(LIBTAME) $(LIBSFSMISC) $(LIBSVC_ME) $(LIBSFSCRYPT) $(LIBARPC) $(LIBSAFEPTR) $(LIBASYNC) $(LIBGMP)

tinetd_LDADD = $(LDADD)

TAMEIN = tinetd.T
TAMEOUT = tinetd.C

SUFFIXES = .x .T
.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)

CLEANFILES = core *.core *~ *_config *_log $(TAMEOUT)
EXTRA_DIST = $(TAMEIN) 
MAINTAINERCLEANFILES = Makefile.in

.PHONY:	tameclean rpcclean

tameclean:
	rm -f $(TAMEOUT)

dist-hook:
	cd $(distdir) && rm -f $(TAMEOUT)

