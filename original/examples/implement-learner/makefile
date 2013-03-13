# list all your source files here
SOURCES= implement-learner.c

# update these for your working directory
INCLUDE_DIRS = -I ../../include/
LIB = ../../lib/vfml.a -lm

# the name you would like the program to have
PROGRAM = implement-learner

# you might care about this
CC_FLAGS = -O2 -g

##################################
# you probably won't want to change anything below this
##################################

OBJECTS = $(SOURCES:.c=.o) 
CFLAGS = ${CC_FLAGS} ${INCLUDE_DIRS}


all: ${PROGRAM}

${PROGRAM}: ${OBJECTS} ${LIB}
	gcc ${CC_FLAGS} ${OBJECTS} ${LIB} -o $@

.PHONY : clean
clean: 
	rm -r -f *~ ${OBJECTS}

.PHONY : clobber
clobber: clean
	rm -r -f ${PROGRAM} 

