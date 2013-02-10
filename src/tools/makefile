# update these for your working directory
INCLUDE_DIRS = -I ../../include/
LIB = ../../lib/vfml.a -lm
BINDIR = ../../bin/

ifeq (${ARCH}, CYGNUS)
	BINARY_EXT = .exe
else
	BINARY_EXT = 
endif

# the names of all the util programs
PROGRAMS = folddata xvalidate batchtest shuffledata splitdata treedata sampledata clusterdata bindata cleandata uRunner beliefnetdata beliefnetcorrupt beliefnetscore combinedata

# you might care about this
CFLAGS = ${GLOBALCFLAGS} -D${ARCH} ${INCLUDE_DIRS}

all: ${PROGRAMS} ${LIB}

.PHONY : uRunner
uRunner:
	echo "#!${PYTHON}" > ${BINDIR}/uRunner
	cat uRunnerDir/uRunner >> ${BINDIR}/uRunner
	chmod 755 ${BINDIR}/uRunner
	echo "#!${PYTHON}" > ${BINDIR}/uRunnerClient
	cat uRunnerDir/uRunnerClient >> ${BINDIR}/uRunnerClient
	chmod 755 ${BINDIR}/uRunnerClient
	cp  uRunnerDir/JobQueue.py ${BINDIR}

cleandata: cleandata.c ${LIB} makefile
	${CC} ${CFLAGS} cleandata.c ${LIB} -o $@
	cp $@${BINARY_EXT} ${BINDIR}

sampledata: sampledata.c ${LIB} makefile
	${CC} ${CFLAGS} sampledata.c ${LIB} -o $@
	cp $@${BINARY_EXT} ${BINDIR}

treedata: treedata.c ${LIB} makefile
	${CC} ${CFLAGS} treedata.c ${LIB} -o $@
	cp $@${BINARY_EXT} ${BINDIR}

beliefnetdata: beliefnetdata.c ${LIB} makefile
	${CC} ${CFLAGS} beliefnetdata.c ${LIB} -o $@
	cp $@${BINARY_EXT} ${BINDIR}

beliefnetcorrupt: beliefnetcorrupt.c ${LIB} makefile
	${CC} ${CFLAGS} beliefnetcorrupt.c ${LIB} -o $@
	cp $@${BINARY_EXT} ${BINDIR}

beliefnetscore: beliefnetscore.c ${LIB} makefile
	${CC} ${CFLAGS} beliefnetscore.c ${LIB} -o $@
	cp $@${BINARY_EXT} ${BINDIR}

clusterdata: clusterdata.c ${LIB} makefile
	${CC} ${CFLAGS} clusterdata.c ${LIB} -o $@
	cp $@${BINARY_EXT} ${BINDIR}

bindata: bindata.c ${LIB} makefile
	${CC} ${CFLAGS} bindata.c ${LIB} -o $@
	cp $@${BINARY_EXT} ${BINDIR}

splitdata: splitdata.c ${LIB} makefile
	${CC} ${CFLAGS} splitdata.c ${LIB} -o $@
	cp $@${BINARY_EXT} ${BINDIR}

combinedata: combinedata.c ${LIB} makefile
	${CC} ${CFLAGS} combinedata.c ${LIB} -o $@
	cp $@${BINARY_EXT} ${BINDIR}

shuffledata: shuffledata.c ${LIB} makefile
	${CC} ${CFLAGS} shuffledata.c ${LIB} -o $@
	cp $@${BINARY_EXT} ${BINDIR}

folddata: folddata.c ${LIB} makefile
	${CC} ${CFLAGS} folddata.c ${LIB} -o $@
	cp $@${BINARY_EXT} ${BINDIR}

xvalidate: xvalidate.c ${LIB} makefile
	${CC} ${CFLAGS} xvalidate.c ${LIB} -lm -o $@
	cp $@${BINARY_EXT} ${BINDIR}

batchtest: batchtest.c ${LIB} makefile
	${CC} ${CFLAGS} batchtest.c ${LIB} -o $@
	cp $@${BINARY_EXT} ${BINDIR}

.PHONY : clean
clean: 
	rm -r -f *~ *.o

.PHONY : clobber
clobber: clean
	rm -f ../../bin/*
	rm -r -f ${PROGRAMS}
