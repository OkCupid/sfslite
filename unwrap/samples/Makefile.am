## Process this file with automake to produce Makefile.in
# $Id$

LDADD = $(LIBASYNC) 


noinst_PROGRAMS = ex1
ex1_SOURCES = ex1.C


ex1.C:	ex1.W
	../unwrap -o ex1.C $(srcdir)/ex1.W
