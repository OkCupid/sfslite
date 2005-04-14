
#CXX =		g++
CXX =		/usr/local/gcc33/bin/g++
#PRFX =		/usr
PRFX =		/usr/local
#SFSTAG =	shdbg
SFSTAG	=	dbg33
#RPCC =		/disk/max/d/sfslite/rpcc/rpcc
RPCC =		/disk/rael/max/d33/sfs/rpcc/rpcc


PYINC =		-I$(PRFX)/include/python2.3
SFSINCDIR =	/usr/local/include/sfslite/$(SFSTAG)
SFSLIBDIR =	/usr/local/lib/sfslite/shdbg

SFSINC =	-I$(SFSINCDIR) -I/usr/local/include

SFSLIB = 	-L$(SFSLIBDIR) -lasync -L$(SFSLIBDIR) -larpc \
		-L$(SFSLIBDIR) -lsfscrypt -lgmp \
		-Xlinker -rpath $(SFSLIBDIR)

default:	all

all:		core.so

.C.o:
	$(CXX) -g $(PYINC) $(SFSINC) -o $@ -c $< 

core.so: core.o
	$(CXX) -shared $< $(SFSLIB) -o $@

clean:	
	rm -f *.o *.pyc *.so *.core *_wrap.cxx
