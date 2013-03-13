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
PROGRAMS = bnlearn

# you might care about this
CFLAGS = ${GLOBALCFLAGS} -Wall -D${ARCH} ${INCLUDE_DIRS}

all: ${PROGRAMS} ${LIB}

bnlearn: bnlearn.c ${LIB} makefile
	${CC} ${CFLAGS} bnlearn.c ${LIB} -o $@
	cp $@${BINARY_EXT} ${BINDIR}

.PHONY : clean
clean: 
	rm -r -f *~ *.o

.PHONY : clobber
clobber: clean
	rm -r -f ${PROGRAMS} 
	rm -r -f ${BINDIR}bnlearn
