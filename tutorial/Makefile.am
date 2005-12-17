## Process this file with automake to produce Makefile.in
# $Id$

LDADD =  $(LIBARPC) $(LIBASYNC)  

noinst_PROGRAMS = ex4 ex1 ex2 exsrv ex3 ex4m null ex5 ex3b
ex1_SOURCES = ex1.C 
ex2_SOURCES = ex2.C ex_prot.C
ex3_SOURCES = ex3.C ex_prot.C
ex3b_SOURCES = ex3b.C ex_prot.C
ex4_SOURCES = ex4.C ex_prot.C
ex5_SOURCES = ex5.C ex_prot.C
null_SOURCES = null.C
exsrv_SOURCES = exsrv.C ex_prot.C
ex4m_SOURCES = ex4m.C ex_prot.C

SUFFIXES = .T .C .x .h

.T.C:
	-$(TAME) -L -o $@ $< || rm -f $@
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
ex3b.o: ex_prot.h
ex4.o: ex_prot.h
ex5.o: ex_prot.h
exsrv.o: ex_prot.h

ex1.C: $(srcdir)/ex1.T
ex2.C: $(srcdir)/ex2.T
ex3.C: $(srcdir)/ex3.T
ex3b.C: $(srcdir)/ex3b.T
ex4.C: $(srcdir)/ex4.T
ex5.C: $(srcdir)/ex5.T
null.C: $(srcdir)/null.T

.PHONY: bldclean
bldclean:
	rm -f $(CLEANFILES)

EXTRA_DIST = ex1.T ex2.T ex3.T ex3b.T ex4.T ex5.T ex_prot.x null.T

CLEANFILES = core *.core ex1.C ex2.C ex3.C ex3b.C ex4.C ex5.C ex_prot.C \
	ex_prot.h
