## Process this file with automake to produce Makefile.in
# $Id$

LDADD =  $(LIBARPC) $(LIBASYNC)  

noinst_PROGRAMS = ex4 ex1 ex2 exsrv ex3 ex4m null
ex1_SOURCES = ex1.C 
ex2_SOURCES = ex2.C ex_prot.C
ex3_SOURCES = ex3.C ex_prot.C
ex4_SOURCES = ex4.C ex_prot.C
null_SOURCES = null.C
exsrv_SOURCES = exsrv.C ex_prot.C
ex4m_SOURCES = ex4m.C ex_prot.C

SUFFIXES = .T .C .x .h

.T.C:
	-$(UNWRAP) -L -o $@ $< || rm -f $@
.x.h:
	-$(RPCC) -h $< || rm -f $@
.x.C:
	-$(RPCC) -c $< || rm -f $@

ex_prot.C: ex_prot.x
ex_prot.h: ex_prot.x
ex_prot.o: ex_prot.h
ex_prot.lo: ex_prot.h

ex2.o: ex_prot.h
ex3.o: ex_prot.h
ex4.o: ex_prot.h
exsrv.o: ex_prot.h

ex3.C: $(srcdir)/ex3.T
ex1.C: $(srcdir)/ex1.T
ex2.C: $(srcdir)/ex2.T
ex4.C: $(srcdir)/ex4.T
null.C: $(srcdir)/null.T

.PHONY: bldclean
bldclean:
	rm -f ex1.C ex2.C ex3.C ex4.C ex_prot.C ex_prot.h
