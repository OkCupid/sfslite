## Process this file with automake to produce Makefile.in
# $Id$

LDADD = $(LIBASYNC)  $(LIBARPC)

noinst_PROGRAMS = ex1 ex2 ex2srv
ex1_SOURCES = ex1.C 
ex2_SOURCES = ex2.C ex2_prot.C
ex2srv_SOURCES = ex2_srv.C ex2_prot.C

SUFFIXES = .W .C .x .h

.W.C:
	-$(UNWRAP) -o $@ $< || rm -f $@
.x.h:
	-$(RPCC) -h $< || rm -f $@
.x.C:
	-$(RPCC) -c $< || rm -f $@

ex2_prot.C: ex2_prot.x
ex2_prot.h: ex2_prot.x
ex2_prot.o: ex2_prot.h
ex2_prot.lo: ex2_prot.h

ex1.C: ex1.W
ex2.o: ex2_prot.h
ex2_srv.o: ex2_prot.h
