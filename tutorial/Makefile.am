## Process this file with automake to produce Makefile.in
# $Id$

LDADD =  $(LIBARPC) $(LIBASYNC)  

noinst_PROGRAMS = ex1 ex2 ex3 ex3b ex4m ex4 ex5 ex6 ex7 ex8 ex9 exsrv null \
		buggy1a buggy1b buggy1c ex10
ex1_SOURCES = ex1.C 
ex2_SOURCES = ex2.C ex_prot.C
ex3_SOURCES = ex3.C ex_prot.C
ex3b_SOURCES = ex3b.C ex_prot.C
ex4_SOURCES = ex4.C ex_prot.C
ex5_SOURCES = ex5.C ex_prot.C
ex6_SOURCES = ex6.C
ex7_SOURCES = ex7.C
ex8_SOURCES = ex8.C
ex9_SOURCES = ex9.C ex_prot.C
ex10_SOURCES = ex10.C ex_prot.C
buggy1a_SOURCES = buggy1a.C
buggy1b_SOURCES = buggy1b.C
buggy1c_SOURCES = buggy1c.C
null_SOURCES = null.C
exsrv_SOURCES = exsrv.C ex_prot.C
ex4m_SOURCES = ex4m.C ex_prot.C

SUFFIXES = .T .C .x .h

##
## compile with the -L flag to suppress line # translation
##
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
ex9.o: ex_prot.h
ex10.o: ex_prot.h
exsrv.o: ex_prot.h

ex1.C: $(srcdir)/ex1.T
ex2.C: $(srcdir)/ex2.T
ex3.C: $(srcdir)/ex3.T
ex3b.C: $(srcdir)/ex3b.T
ex4.C: $(srcdir)/ex4.T
ex5.C: $(srcdir)/ex5.T
ex6.C: $(srcdir)/ex6.T
ex7.C: $(srcdir)/ex7.T
ex8.C: $(srcdir)/ex8.T
ex9.C: $(srcdir)/ex9.T
ex10.C: $(srcdir)/ex10.T
buggy1a.C: $(srcdir)/buggy1a.T
buggy1b.C: $(srcdir)/buggy1b.T
buggy1c.C: $(srcdir)/buggy1c.T
null.C: $(srcdir)/null.T

dist-hook:
	cd $(distdir) && rm -f $(CLEANFILES)

.PHONY: bldclean
bldclean:
	rm -f $(CLEANFILES)

EXTRA_DIST = ex1.T ex2.T ex3.T ex3b.T ex4.T ex5.T ex_prot.x null.T ex6.T \
	ex7.T ex8.T ex9.T buggy1a.T buggy1b.T buggy1c.T ex10.T

CLEANFILES = core *.core ex1.C ex2.C ex3.C ex3b.C ex4.C ex5.C ex_prot.C \
	ex_prot.h null.C ex6.C ex7.C ex8.C ex9.C buggy1a.C buggy1b.C \
	buggy1c.C ex10.C
