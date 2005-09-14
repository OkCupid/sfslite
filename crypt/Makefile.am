## Process this file with automake to produce Makefile.in
# $Id$

LDADD = $(LIBSFSCRYPT) $(LIBARPC) $(LIBASYNC) $(LIBGMP) $(LIBPY)

sfslib_LTLIBRARIES = libsfscrypt.la
LIBSFSCRYPT = libsfscrypt.la

libsfscrypt_la_SOURCES = \
aes.C arc4.C axprt_crypt.C blowfish.C crypt_prot.C esign.C		\
getkbdnoise.C getsysnoise.C hashcash.C mdblock.C modalg.C mpscrub.C	\
mpz_misc.C mpz_raw.C mpz_square.C mpz_xor.C pad.C password.C prng.C	\
rabin.C random_prime.C rndseed.C rsa.C seqno.C sha1.C sha1oracle.C	\
tiger.C tiger_sboxes.C wmstr.C xdr_mpz_t.C schnorr.C ocb.C 

sfsinclude_HEADERS = crypt_prot.x \
aes.h arc4.h axprt_crypt.h bench.h bigint.h blowfish.h crypt.h		\
crypthash.h crypt_prot.h esign.h hashcash.h modalg.h password.h		\
prime.h prng.h rabin.h rsa.h seqno.h sha1.h tiger.h wmstr.h schnorr.h	\
ocb.h 

noinst_HEADERS = blowfish_data.h

crypt_prot.h: $(srcdir)/crypt_prot.x
	@rm -f $@
	-$(RPCC) -h $(srcdir)/crypt_prot.x || rm -f $@

crypt_prot.C: $(srcdir)/crypt_prot.x
	@rm -f $@
	-$(RPCC) -c $(srcdir)/crypt_prot.x || rm -f $@

dist-hook:
	cd $(distdir) && rm -f crypt_prot.h crypt_prot.C

srp.o srp.lo: crypt_prot.h
crypt_prot.o crypt_prot.lo: crypt_prot.h

noinst_PROGRAMS = tst timing
tst_SOURCES = tst.C
timing_SOURCES =  timing.C
tst_LDADD = $(LDADD)
tst_DEPENDENCIES = $(LIBSFSCRYPT) $(LIBARPC) $(LIBASYNC)

if REPO
sfscrypt_repo_OBJECTS = $(libsfscrypt_la_OBJECTS) \
	$(LIBARPC) $(LIBASYNC)
stamp-sfscrypt-repo: $(sfscrypt_repo_OBJECTS)
	-$(CXXLINK) $(sfscrypt_repo_OBJECTS)
	@rm -f a.out
	touch $@
libsfscrypt_la_DEPENDENCIES = stamp-sfscrypt-repo
endif

.PHONY: rpcclean
rpcclean:
	rm -f crypt_prot.h crypt_prot.C

EXTRA_DIST = crypt_prot.x .cvsignore
CLEANFILES = core *.core *~ *.rpo stamp-sfscrypt-repo \
		crypt_prot.h crypt_prot.C
MAINTAINERCLEANFILES = Makefile.in
