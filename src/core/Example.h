#ifndef EXAMPLEH
#define EXAMPLEH

/** \ingroup CoreAPI DecisionTree BeliefNet Clustering
*/

#include "ExampleSpec.h"
#include "../util/lists.h"

/** \file

\brief ADT for training (and testing, etc.) data.

This is the interface for working with instances of the Example ADT.
Note that all access to attributes is 0 based (just like C arrays).

Note that all the Examples created with an ExampleSpec maintain a
pointer to the ExampleSpec, so you shouldn't free it or modify the
ExampleSpec until you are done with all the Examples referencing it.

*/

typedef union {
   short discreteValue;
   float continuousValue;
} AttributeValue;

typedef enum {
   Discrete,
   Continuous
} AttributeType;

/** \brief ADT for working with examples. 

See Example.h for more detail.
*/
typedef struct _Example_ {
   int myclass;
   ExampleSpecPtr spec;

   VoidAListPtr attributes;
} Example, *ExamplePtr;

/*************************/
/* construction routines */
/*************************/

/** \brief Programmatically creates a new example.

Allocates enough memory to hold all the attributes mentioned in the
ExampleSpec passed. Use the ExampleSetFoo functions (see below) to
set the values of the attributes and class.

This function allocates memory which should be freed by calling ExampleFree.
*/
ExamplePtr ExampleNew(ExampleSpecPtr es);
ExamplePtr ExampleNewInitUnknown(ExampleSpecPtr es);


/** \brief Frees all the memory being used by the passed example. */
void ExampleFree(ExamplePtr e);

/** \brief Allocates memory and copies the example into it. */
ExamplePtr ExampleClone(ExamplePtr e);

/** \brief Marks the specified attribute value as unknown.

Future calls to ExampleGetAttributeUnknown for that attribute will return 1.
*/
void ExampleSetAttributeUnknown(ExamplePtr e, int attNum);


/** \brief Considers the specified attribute to be discrete and sets its value to the specified value.

This function doesn't do much error checking so be sure that you call
it consistent with ExampleIsAttributeDiscrete,
ExampleIsAttributeContinuous and ExampleSpecGetAttributeValueCount.
If you don't, there is a chance the example could cause your program
to crash.
*/
void ExampleSetDiscreteAttributeValue(ExamplePtr e, int attNum, int value);

/** \brief Considers the specified attribute to be continuous and sets its value to the specified value.

This function doesn't do much error checking so be sure that you call
it consistent with ExampleIsAttributeDiscrete, and
ExampleIsAttributeContinuous.  If you don't, there is a chance the
example could cause your program to crash.
*/
void ExampleSetContinuousAttributeValue(ExamplePtr e, int attNum, double value);

/** \brief Sets the example's class to the specified class.

The function doesn't do much error checking so be sure that you call
it consistent with ExampleSpecGetNumClasses.  If you don't, there is a
chance the example could cause your program to crash.
*/
void ExampleSetClass(ExamplePtr e, int theClass);


/** \brief Marks the example's class as unknown. */
void ExampleSetClassUnknown(ExamplePtr e);

/** \brief Randomly corrupts the attributes and class of the example.

p should be a number between 0-1, which is interpreted as a
probability (e.g. a value of .732 would be interpreted as 73.2%).
class and attrib are flags which should be 1 if you want noise added
to that part of the example and 0 otherwise.  Then, for each discrete
thing selected by the flags, this function will have the specified
probability of changing it, without replacement, to a randomly
selected value.  This function changes the value of each continuous
attribute by adding to it a value drawn from a normal distribution
with mean 0 and with standard deviation p.

*/
void ExampleAddNoise(ExamplePtr e, float p, int doClass, int attrib);

/** \brief Attempts to read an example from the passed file pointer.

FILE * should be opened for reading. The file should contain examples
in C4.5 format. Uses the ExampleSpec to determine how many, what
types, and how to interpret the attributes it needs to read.

This function will return 0 (NULL) if it is unable to read an example
from the file (bad input or EOF).  If the input is badly formed, the
function will also output an error to the console.

Note that you could pass STDIN to the function to read an example from
the console.

This function allocates memory which should be freed by calling ExampleFree.
*/
ExamplePtr ExampleRead(FILE *file, ExampleSpecPtr es);

/** \brief Reads as many examples as possible from the file pointer.

Calls ExampleRead until it gets a 0, allocates a list and adds each
example to it.  You are responsible for freeing the examples and the
list.
*/
VoidListPtr ExamplesRead(FILE *file, ExampleSpecPtr es);

/*************************/
/* Accessor routines     */
/*************************/

/** \brief Returns 1 if the specified attribute is discrete and 0 otherwise.

Be sure not to ask for an attNum >= ExampleGetNumAttributes(e).
*/
int ExampleIsAttributeDiscrete(ExamplePtr e, int attNum);

/** \brief Returns 1 if the specified attribute is continuous and 0 otherwise.

Be sure not to ask for an attNum >= ExampleGetNumAttributes(e).
*/
int ExampleIsAttributeContinuous(ExamplePtr e, int attNum);

//int ExampleIsAttributeUnknown(ExamplePtr e, int attNum);
/** \brief Returns 1 if the specified attribute is marked as unknown and 0 otherwise.

Be sure not to ask for an attNum >= ExampleGetNumAttributes(e).
*/
#define ExampleIsAttributeUnknown(e, attNum) \
   ( VALIndex(e->attributes, attNum) == 0 )

//int ExampleGetNumAttributes(ExamplePtr e);
/** \brief Returns the number of attributes that this example has.

This will be equal to the number of attributes that were in the
ExampleSpec used to construct the example.
*/
#define ExampleGetNumAttributes(e) \
  (VALLength(((ExamplePtr)e)->attributes))

/** \brief Returns the value of the indicated discrete attribute.

If the attNum is valid, and ExampleGetAttributeUnknown(e, attNum)
returns 0, and ExampleIsAttributeDiscrete(e, attNum) returns 1, this
function will return the value of the attribute.  If the conditions
aren't met, there is a good chance that calling this will crash your
program.
*/
int ExampleGetDiscreteAttributeValue(ExamplePtr e, int attNum);

/** \brief Returns the value of the indicated continuous attribute. 

If the attNum is valid, and ExampleGetAttributeUnknown(e, attNum)
returns 0, and ExampleIsAttributeContinuous(e, attNum) returns 1, this
function will return the value of the attribute.  If the conditions
aren't met, there is a good chance that calling this will crash your
program.
*/
double ExampleGetContinuousAttributeValue(ExamplePtr e, int attNum);

//inline int ExampleGetDiscreteAttributeValueUnsafe(ExamplePtr e, int attNum) {
//    return ((AttributeValue *)VALIndex(e->attributes, attNum))->discreteValue;
//}

#define ExampleGetDiscreteAttributeValueUnsafe(e,attNum) \
   (((AttributeValue *)VALIndex((e)->attributes, (attNum)))->discreteValue)


//int ExampleGetClass(ExamplePtr e);
/** \brief Returns the value of the example's class.

If the value of the example's class is known this returns the value,
otherwise this returns -1.
*/
#define ExampleGetClass(e) \
     (((ExamplePtr)e)->myclass)

/** \brief Returns 1 if the value of the example's class is known, and
0 otherwise. */
int ExampleIsClassUnknown(ExamplePtr e);

/** \brief Returns the euclidian distance between the two examples.

This ignores discrete attributes.
*/
float ExampleDistance(ExamplePtr e, ExamplePtr dst);

/*************************/
/* Display routines      */
/*************************/

/** \brief Writes the example to the passed file point.er

FILE * should be opened for writing. The example will be written in
C4.5 format, and could later be read in using ExampleRead.

Note that you could pass stdout to the function to write an example to
the console.
*/
void ExampleWrite(ExamplePtr e, FILE *out);

#endif 
