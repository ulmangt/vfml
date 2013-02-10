VPATH = util/ core/ learners/bnlearn/

SOURCES= fc45.tab.c lex.fc45.c bif.tab.c lex.bif.c \
         Debug.c lists.c memory.c random.c bitfield.c stats.c \
         ExampleSpec.c Example.c DecisionTree.c getnames.c \
         AttributeTracker.c ExampleGenerator.c ExampleGroupStats.c \
         REPrune.c C45interface.c HashTable.c  BeliefNet.c bnlearn-engine.c

OBJECTS = $(SOURCES:.c=.o) 
INCLUDE_DIRS = -I util -I core -I ../../include -I ../include -I . -I learners/bnlearn

CFLAGS = ${GLOBALCFLAGS} -D${ARCH} ${INCLUDE_DIRS} 

# add -pg above to turn on profiling

LIBRARY = vfml.a
LIBRARY2 = libvfml.a

all: ${LIBRARY} $(LIBRARY2) tools learners

lib: $(LIBRARY) $(LIBRARY2)

$(LIBRARY2): $(LIBRARY)
	cp $(LIBRARY) ../lib/$(LIBRARY2)

${LIBRARY}: ${OBJECTS}
	ar -r ${LIBRARY} ${OBJECTS}
	ranlib ${LIBRARY}
	cp ${LIBRARY} ../lib/

.PHONY : tools
tools: ${LIBRARY}
	(cd tools ; ${MAKE})

.PHONY : learners
learners: ${LIBRARY}
	(cd learners ; ${MAKE})

fc45.tab.c: core/fc45.y
	bison -d -p fc45 core/fc45.y

lex.fc45.c: core/fc45.lex
	flex -Pfc45 core/fc45.lex

bif.tab.c: core/bif.y
	bison -d -p bif core/bif.y

lex.bif.c: core/bif.lex
	flex -Pbif core/bif.lex

.PHONY : clean
clean: 
	rm -r -f *~ ${OBJECTS} fc45.tab.c lex.fc45.c fc45.tab.h
	rm -r -f bif.tab.c lex.bif.c bif.tab.h
	(cd tools ; ${MAKE} clean)
	(cd learners ; ${MAKE} clean)

.PHONY : clobber
clobber: clean
	rm -f ${LIBRARY} ../lib/${LIBRARY}
	rm -f ${LIBRARY} ../lib/${LIBRARY2}
	(cd tools ; ${MAKE} clobber)
	(cd learners ; ${MAKE} clobber)
