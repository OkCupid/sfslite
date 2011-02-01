
sfsbin_PROGRAMS = ncpp
ncpp_SOURCES = ncpp.C

if USE_SFSMISC
LIBSVC_ME = $(LIBSVC)
else
LIBSVC_ME =
endif

TAMEIN = ncpp.T
TAMEOUT = ncpp.C

SUFFIXES = .x .T
.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)

LDADD = $(LIBAAPP) $(LIBTAME) $(LIBSFSMISC) $(LIBSVC_ME) $(LIBSFSCRYPT) $(LIBARPC) $(LIBSAFEPTR) $(LIBASYNC) $(LIBGMP) $(LDADD_STD_ALL)

ncpp_LDADD = $(LDADD)

CLEANFILES = core *.core *~ *_config *_log $(TAMEOUT)
EXTRA_DIST = $(TAMEIN) 
MAINTAINERCLEANFILES = Makefile.in

.PHONY:	tameclean rpcclean

tameclean:
	rm -f $(TAMEOUT) $(TAMEOUT)

dist-hook:
	cd $(distdir) && rm -f $(TAMEOUT)

