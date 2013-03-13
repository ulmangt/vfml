#ifndef EXAMPLEGENERATORH
#define EXAMPLEGENERATORH

/** \ingroup CoreAPI
*/

#include "ExampleSpec.h"
#include "Example.h"


/** \file

\brief Generate a random (but reproducible) data set

Randomly, but reproducably, create a series of examples.  These
examples could then be classified with some known model and used as a
synthetic dataset to test a learner.  This uses the RandomSetState()
functions so that it will produce the same series of examples for the
same seed no matter what the rest of your program does with the random
number generators.

*/

/** \brief Holds the information needed to reproducibly make a random data set. 
See ExampleGenerator.h for more detail.
*/
typedef struct _ExampleGenerator_ {
   void *randomState;
   ExampleSpecPtr spec;
   long numGenerated;
   long maxGenerated;
} ExampleGenerator, *ExampleGeneratorPtr;


/** \brief Creates a new example generator.

The generator will generate examples conforming to es using a seeded
random number generator. If the value of seed is -1 this will select a
random initial seed (But you'll need to initialize the random number
generator on your system for this to work; The function RandomInit()
will do the job).
*/
ExampleGeneratorPtr ExampleGeneratorNew(ExampleSpecPtr es, int seed);

/** \brief Frees the memory associated with the example generator. */
void ExampleGeneratorFree(ExampleGeneratorPtr eg);

/** \brief Makes a random example.

Allocates an example, randomly sets the values of its attributes, and
returns it.  Uses uniform distributions for all of its decisions.  For
continuous attributes it uniformly generates a value between 0 and
1.0; you might like to scale this value to fit your needs.

You are must free the ExamplePtr when you are finished with it by
calling ExampleFree.
*/
ExamplePtr ExampleGeneratorGenerate(ExampleGeneratorPtr eg);


#endif /* EXAMPLEGENERATORH */
