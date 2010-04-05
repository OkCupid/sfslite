
sfsbin_PROGRAMS = sfs_logger
sfs_logger_SOURCES = logger.C

if USE_SFSMISC
LIBSVC_ME = $(LIBSVC)
else
LIBSVC_ME =
endif

LDADD = $(LIBAAPP) $(LIBTAME) $(LIBSFSMISC) $(LIBSVC_ME) $(LIBSFSCRYPT) $(LIBARPC) $(LIBSAFEPTR) $(LIBASYNC) $(LIBGMP) $(LDADD_STD_ALL)

sfs_logger_LDADD = $(LDADD)

CLEANFILES = core *.core *~ *_config *_log $(TAMEOUT)
EXTRA_DIST = $(TAMEIN) 
MAINTAINERCLEANFILES = Makefile.in

.PHONY:	tameclean rpcclean

tameclean:
	rm -f $(TAMEOUT)

dist-hook:
	cd $(distdir) && rm -f $(TAMEOUT)

