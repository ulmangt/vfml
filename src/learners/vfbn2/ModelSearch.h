#ifndef MODELSEARCHH
#define MODELSEARCHH

#include "vfml.h"

typedef struct _ModelSearch_ {
   long initialExampleNumber;
   int doneCollectingExamples;
   VoidListPtr choices;

   /* will be 1 adding nothing beats any of the alternatives */
   int isDone;

   /* for memory optimizations */
   int lastParentAdded;
   int isActive;

   /* which node is this search for */
   int nodeID;
   BeliefNet currentModel;

   /* search loop control */
   int *linkChangeCounts;

   /* count search alternatives */
   int numSearchOptions;

} ModelSearchStruct, *ModelSearch;

/* create a new search for the node indicated, starts inactive,
      call activate right away if there is enough memory */
ModelSearch MSNew(BeliefNet currentBN, int nodeID);

void MSFree(ModelSearch ms);

int MSGetNodeID(ModelSearch ms);

void MSAddExample(ModelSearch ms, ExamplePtr e, long exampleNumber);

/* tell the search it isn't getting any more examples, so
     the next time CheckForWinner is called it should just pick the best */
void MSTerminateSearch(ModelSearch ms);

int MSIsDone(ModelSearch ms);

int  MSCheckForWinner(ModelSearch ms);

void MSModelChangedHandleConflicts(ModelSearch ms);

/* a deactive search frees the memory being used for search alternatives
       and only updates current model parameters with future examples */
int MSIsActive(ModelSearch ms);
void MSActivate(ModelSearch ms);
void MSDeactivate(ModelSearch ms);
int MSNumActiveAlternatives(ModelSearch ms);

#endif
