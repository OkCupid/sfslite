
CXX =		g++
#CXX =		/usr/local/gcc33/bin/g++
PRFX =		/usr
#PRFX =		/usr/local
SFSTAG =	shdbg
#SFSTAG	=	dbg33
RPCC =		/disk/max/d/sfslite/rpcc/rpcc
#RPCC =		/disk/rael/max/d33/sfs/rpcc/rpcc


PYINC =		-I$(PRFX)/include/python2.3
SFSINCDIR =	/usr/local/include/sfslite/$(SFSTAG)
SFSLIBDIR =	/usr/local/lib/sfslite/$(SFSTAG)

SFSINC =	-I$(SFSINCDIR) -I/usr/local/include

SFSLIB = 	-L$(SFSLIBDIR) -lasync -L$(SFSLIBDIR) -larpc \
		-L$(SFSLIBDIR) -lsfscrypt -lpyarpc -lgmp \
		-Xlinker -rpath $(SFSLIBDIR)

default:	ex1.so

ex1.C:	ex1.x
	$(RPCC) -q -o $@ $<

%.so: %.o
	$(CXX) -shared $< $(SFSLIB) -o $@

.C.o:
	$(CXX) -g $(PYINC) $(SFSINC) -o $@ -c $< 

clean:	
	rm -f *.o *.pyc *.so *.core *_wrap.cxx ex1.C
