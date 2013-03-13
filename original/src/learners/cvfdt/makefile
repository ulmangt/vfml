# update these for your working directory
INCLUDE_DIRS = -I ../../../include/
LIB = ../../../lib/vfml.a -lm
BINDIR = ../../../bin/

ifeq (${ARCH}, CYGNUS)
	BINARY_EXT = .exe
else
	BINARY_EXT = 
endif

# the name you would like the program to have
PROGRAMS = cvfdt

# you might care about this
CFLAGS = ${GLOBALCFLAGS} -D${ARCH} ${INCLUDE_DIRS}

all: ${PROGRAMS} ${LIB}

cvfdt: vfdt-engine.c vfdt.c cvfdt.c ${LIB} makefile
	${CC} ${CFLAGS} vfdt-engine.c cvfdt.c ${LIB} -o $@
	cp $@${BINARY_EXT} ${BINDIR}

.PHONY : clean
clean: 
	rm -r -f *~ *.o

.PHONY : clobber
clobber: clean
	rm -r -f ${PROGRAMS} 
	rm -r -f ${BINDIR}cvfdt
