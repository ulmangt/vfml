# This mainly affects the file extension, in CYGNUS the binary files
# need to end with .exe
export ARCH = UNIX
#export ARCH = CYGNUS

export PYTHON=/usr/bin/python


# Choose a compiler if you like
#export CC=icc
export CC=gcc


# Any flags, for now this has debugging, optimization, and some stuff
#  to allow VFML to work with large files on some versions of LINUX.

export GLOBALCFLAGS = -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE64 -O3 -g -Wall 

# add -pg above to profile also add it in src/makefile where commented


# You probably won't want to change things below here
all:
	(cd src ; ${MAKE} all)

.PHONY : lib
lib: 
	(cd src ; ${MAKE} lib)

.PHONY : clean
clean: 
	rm -r -f *~
	(cd src ; ${MAKE} clean)

.PHONY : clobber
clobber: clean
	(cd src ; ${MAKE} clobber)

