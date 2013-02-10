#ifndef EXAMPLESPECH
#define EXAMPLESPECH

/** \ingroup CoreAPI DecisionTree BeliefNet Clustering
*/

/** \file 

\brief Schema for training data.

This is based off of Ross Quinlan's <a href="appendixes/c45.htm">C4.5
format</a> and can read and write that format to disk.

\bug Under Cygnus (and windows) lex doesn't seem to do the right thing
with EOF rules, so you need to put an extra return at the end of your
.names files.

*/


#include <stdio.h>
#include "../util/lists.h"

typedef enum { asIgnore, 
               asContinuous,
               asDiscreteNamed,
               asDiscreteNoName } AttributeSpecType;

typedef struct _AttributeSpec_ {
   AttributeSpecType type;
   char *attributeName;

   int numDiscreteValues;
   VoidAListPtr attributeValues;
} AttributeSpec, *AttributeSpecPtr;

/*************************/
/* construction routines */
/*************************/
AttributeSpecPtr AttributeSpecNew(void);
void AttributeSpecFree(AttributeSpecPtr as);
void AttributeSpecSetType(AttributeSpecPtr as, AttributeSpecType type);
void AttributeSpecSetName(AttributeSpecPtr as, char *name);
   /* Set name takes the memory of the name over, but check */
void AttributeSpecSetNumValues(AttributeSpecPtr as, int num);
void AttributeSpecAddValue(AttributeSpecPtr as, char *value);

/*************************/
/* Accessor routines     */
/*************************/
//AttributeSpecType AttributeSpecGetType(AttributeSpecPtr as);
#define AttributeSpecGetType(as) (as->type)


//int AttributeSpecGetNumValues(AttributeSpecPtr as);
#define AttributeSpecGetNumValues(as) \
   (((AttributeSpecPtr)as)->numDiscreteValues)

int AttributeSpecGetNumAssignedValues(AttributeSpecPtr as);
char *AttributeSpecGetName(AttributeSpecPtr as);
//char *AttributeSpecGetValueName(AttributeSpecPtr as, int index);
#define AttributeSpecGetValueName(as, index) \
            (VALIndex(as->attributeValues, index))

int AttributeSpecLookupName(AttributeSpecPtr as, char *name);

/*************************/
/* Display routines      */
/*************************/
void AttributeSpecWrite(AttributeSpecPtr as, FILE *out);


/** \brief Schema for training data. */
typedef struct _ExampleSpec_ {
   VoidAListPtr classes;
   VoidAListPtr attributes;
} ExampleSpec, *ExampleSpecPtr;

/*************************/
/* construction routines */
/*************************/

/** \brief Programmatically creates a new ExampleSpec.

Use the ExampleSpecAddFOO functions to add classes, attributes, and
their values to the spec.

This function allocates memory which should be freed by calling
ExampleSpecFree.
*/
ExampleSpecPtr ExampleSpecNew(void);

/** \brief Frees all the memory being used by the ExampleSpec.

Note that all the Examples created with an ExampleSpec maintain a
pointer to the ExampleSpec, so you shouldn't free it or modify the
ExampleSpec until you are done with all the Examples referencing it.
*/
void ExampleSpecFree(ExampleSpecPtr es);

/** \brief Adds a new class to the ExampleSpec and gives it the specified name.

The main use of the name is to read/write Examples and ExampleSpecs in
human readable format. The new class is assigned a value which you can
retrieve by calling: ExampleSpecLookupClassName(es, className).

Note that this function takes over the memory associated with the
className argument and will free it later. This means that you
shouldn't pass in static strings, or strings that were allocated on
the stack.
*/
void ExampleSpecAddClass(ExampleSpecPtr es, char *className);

void ExampleSpecAddAttributeSpec(ExampleSpecPtr es, AttributeSpecPtr as);
  /* NOTE AddAttributeSpec is not part of the external interface. */
  /*  it is only for internal library use. */

/** \brief Adds a new discrete attribute to the ExampleSpec and gives it the specified name.

The main use of the name is to read/write ExampleSpecs in human
readable format. The function returns the index of the new attribute,
you can use the index to add values to the attribute using ExampleSpecAddAttributeValue.

Note that this function takes over the memory associated with the name
argument and will free it later. This means that you shouldn't pass in
static strings, or strings that were allocated on the stack.
*/
int ExampleSpecAddDiscreteAttribute(ExampleSpecPtr es, char *name);


/** \brief Adds a new continuous attribute to the ExampleSpec and gives it the specified name.

The main use of the name is to read/write ExampleSpecs in human
readable format. The function returns the index of the new attribute.

Note that this function takes over the memory associated with the name
argument and will free it later.  This means that you shouldn't pass
in static strings, or strings that were allocated on the stack.

*/
int ExampleSpecAddContinuousAttribute(ExampleSpecPtr es, char *name);

/** \brief Adds a new value to the specified attribute and gives it
the specified name.

The specified attribute had better be a discrete attribute. The main
use of the name is to read/write Examples and ExampleSpecs in human
readable format.

Note that this function takes over the memory associated with the name
argument and will free it later.  This means that you shouldn't pass
in static strings, or strings that were allocated on the stack.

*/
void ExampleSpecAddAttributeValue(ExampleSpecPtr es, int attNum, char *name);



/** \brief Reads a .names formated file and build an ExampleSpec.

Attempts to read an example from the passed FILE *, which should be
opened for reading. The file should contain an ExampleSpec in C4.5
format, that is the file should be a C4.5 names file.

This function will return 0 (NULL) if it is unable to read an
ExampleSpec from the file (bad input or the file does not exist).  If
the input is badly formed, the function will also output an error to
the console.

Note that you could pass stdin to the function to read an ExampleSpec
from the console.

This function allocates memory which should be freed by calling
ExampleSpecFree.
*/
ExampleSpecPtr ExampleSpecRead(char *fileName);
void ExampleSpecIgnoreAttribute(ExampleSpecPtr es, int num);

/*************************/
/* Accessor routines     */
/*************************/

/** \brief Returns the number of attributes that the example spec contains. 
  
Remember that the attributes are indexed in a 0-based fashion (like C
arrays) so the actual valid index for the attributes will be from 0 to
ExampleSpecGetNumAttributes(es) - 1.
*/
int ExampleSpecGetNumAttributes(ExampleSpecPtr es);


/** \brief Returns the type of the specified attribute.  

There are currently four supported types:
- asIgnore 
- asContinuous 
- asDiscreteNamed 
- asDiscreteNoName 

The programmatic construction interface only supports asIgnore,
asContinuous, and asDiscreteNamed, but the other is needed to fully
support the C4.5 format.
*/
//AttributeSpecType ExampleSpecGetAttributeType(ExampleSpecPtr es, int num);
#define ExampleSpecGetAttributeType(es, num) \
   ( AttributeSpecGetType(((AttributeSpecPtr)VALIndex(es->attributes, num))) )


/** \brief Returns 1 if the specified attribute is discrete and 0 otherwise. 

Be sure not to ask for an attribute numbered >=
ExampleSpecGetNumAttributes(es).
*/
int ExampleSpecIsAttributeDiscrete(ExampleSpecPtr es, int num);

/** \brief Returns 1 if the specified attribute is continuous and 0 otherwise.

Be sure not to ask for an attribute numbered >=
ExampleSpecGetNumAttributes(es).
*/
int ExampleSpecIsAttributeContinuous(ExampleSpecPtr es, int num);

/** \brief Returns 1 if the specified attribute should be ignored and 0 otherwise.

Be sure not to ask for an attribute numbered >= ExampleSpecGetNumAttributes(es).
*/
int ExampleSpecIsAttributeIgnored(ExampleSpecPtr es, int num);

/** \brief Returns the number of values of the attribute.

If the attNum is valid and ExampleSpecIsAttributeDiscrete(es, attNum)
returns 1, this function will return the number of values that
attribute has. Remember that these values will be 0 indexed.
*/
//int ExampleSpecGetAttributeValueCount(ExampleSpecPtr es, int attNum);
#define ExampleSpecGetAttributeValueCount(es, attNum) \
  ( AttributeSpecGetNumValues(((AttributeSpecPtr)VALIndex(es->attributes, attNum))) )


int ExampleSpecGetAssignedAttributesValueCount(ExampleSpecPtr es, int attNum);
/* GetAssignedValueCount is only needed internally for I/O */

/** \brief If the attNum is valid, this returns the name of the associated attribute.*/
char *ExampleSpecGetAttributeName(ExampleSpecPtr es, int attNum);

//char *ExampleSpecGetAttributeValueName(ExampleSpecPtr es,
//                                       int attNum, int valNum);

/** \brief Return the name of the specified value of the specified attribute.

If attNum is valid, and ExampleSpecIsAttributeDiscrete(es, attNum)
returns 1, and valNum is valid, this returns the name of the specified
value of the specified attribute.
*/
#define ExampleSpecGetAttributeValueName(es, attNum, valNum) \
      ( (char *)AttributeSpecGetValueName(((AttributeSpecPtr)VALIndex(es->attributes, attNum)), valNum) )

/** \brief Returns the index of the named attribute.

Does a linear search through the ExampleSpec's attributes looking for
the first one named attributeName. Returns the index of the first
matching attribute or -1 if there is no match.
*/
int ExampleSpecLookupAttributeName(ExampleSpecPtr es, char *valName);

/** \brief Returns the index of the named value.

If attribNum is a valid attribute index this does a linear search
through the associated attribute's values for one named
attributeName. Returns the index or -1 if there is no match.
*/
int ExampleSpecLookupAttributeValueName(ExampleSpecPtr es, int attNum,
                                   char *valName);

/** \brief If name is a valid class name, this returns the index associated with the class. */
int ExampleSpecLookupClassName(ExampleSpecPtr es, char *name);

/** \brief If the classNum is valid, this returns the name of the associated class. */
char *ExampleSpecGetClassValueName(ExampleSpecPtr es, int classNum);

/** \brief Returns the number of classes in the ExampleSpec.

Remember that the classes will have indexes 0 -
ExampleSpecGetNumClasses(es) - 1.
*/
int ExampleSpecGetNumClasses(ExampleSpecPtr es);

/*************************/
/* Display routines      */
/*************************/

/** \brief Outputs the ExampleSpec in .names format 

Writes the example to the passed FILE *, which should be opened for
writing. The example will be written in C4.5 names format, and could
later be read in using ExampleSpecRead.

Note that you could pass stdout to the function to write an
ExampleSpec to the console.
*/
void ExampleSpecWrite(ExampleSpecPtr es, FILE *out);

#endif /* EXAMPLESPECH */
