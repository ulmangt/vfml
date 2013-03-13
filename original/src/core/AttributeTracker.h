#ifndef ATTRIBUTETRACKERH
#define ATTRIBUTETRACKERH

#include "ExampleSpec.h"
#include "../util/bitfield.h"


/** \ingroup CoreAPI DecisionTree
*/

/** \file AttributeTracker.h 

\brief Keep a record of which attributes are active.

Sometimes you need to keep track of which attributes you have already
considered, and which are still available for consideration. The
attribute tracker interface helps you efficiently acomplish this task;
you can think of it as a wrapper around a bit field.

For example, when learning a decision tree, each split on a discrete
attribute deactivates an attribute from further consideration. In this
situation you would deactivate the attribute, and then make one clone
for each child.

*/



typedef struct _AttributeTracker_ {
   BitField bits;
   int numAttributes;
   int numActiveAttributes;
} AttributeTracker, *AttributeTrackerPtr;


/** \brief Allocates memory for a new AttributeTracker structure 

Creates a new attribute tracker with one bit for each of
numAttributes. All the attributes are initially marked as active.

*/
AttributeTrackerPtr AttributeTrackerNew(int numAttributes);

/** \brief Frees the memory associated with the attribute tracker.
*/
void AttributeTrackerFree(AttributeTrackerPtr at);

/** \brief Respects 'ignore's in the ExampleSpec

Creates an attribute tracker to track the attributes in the passed
ExampleSpec. Any of es's attributes marked as ignore will be initially
set to inactive.

*/
AttributeTrackerPtr AttributeTrackerInitial(ExampleSpecPtr es);

/** \brief Creates a copy of the passed AttributeTracker and returns the copy.

*/
AttributeTrackerPtr AttributeTrackerClone(AttributeTrackerPtr at);

/** \brief Marks the specified attribute as active.
*/
void AttributeTrackerMarkActive(AttributeTrackerPtr at, int attNum);

/** \brief Marks the specified attribute as inactive.
*/
void AttributeTrackerMarkInactive(AttributeTrackerPtr at, int attNum);

/** \brief Returns 1 if the specified attribute is active.
*/
int AttributeTrackerIsActive(AttributeTrackerPtr at, int attNum);

/** \brief Returns 1 if none of the attributes are active. 

This is a constant time operation (it does not depend on the number of
attributes).
*/

int AttributeTrackerAreAllInactive(AttributeTrackerPtr at);

/** \brief Returns a count of at's active attributes. 

This is a constant time operation (it does not depend on the number of
attributes).
*/
int AttributeTrackerNumActive(AttributeTrackerPtr at);

#endif /* ATTRIBUTETRACKERH */
