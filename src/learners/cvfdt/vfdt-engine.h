#ifndef VFDTH
#define VFDTH

#include "vfml.h"

typedef struct _VFDTGROWINGDATA_ {
   ExampleGroupStatsPtr egs;
   long seenExamplesCount;
  //   long startedOnExampleNum;
   int seenSinceLastProcess;
   VoidListPtr exampleCache;

   float parentErrorRate;
   int parentClass;

   long seenSinceDeactivated;
   long errorsSinceDeactivated;

   float splitConfidence;
} VFDTGrowingData, *VFDTGrowingDataPtr;

typedef struct _VFDT_ {
   ExampleSpecPtr spec;
   DecisionTreePtr dtree;

   float splitConfidence;
   float tieConfidence;
   int messageLevel;
   int useGini;
   int processChunkSize;
   long maxAllocationBytes;

   unsigned long examplesSeen;
   unsigned long numGrowing;

   int cacheExamples;
   int recoverMinimum;

   int reactivateLeaves;
   float highestDeactivatedIndex;
   unsigned long reactivateScanPeriod;

   int batchMode;
   int prePrune;
   int adaptiveDelta;
   float maxAdaptiveSplitConfidence;
} VFDT, *VFDTPtr;

VFDTPtr VFDTNew(ExampleSpecPtr spec, float splitConfidence,
           float tieConfidence);
void VFDTFree(VFDTPtr vfdt);
void VFDTSetMessageLevel(VFDTPtr vfdt, int value);
void VFDTSetMaxAllocationMegs(VFDTPtr vfdt, int value);
void VFDTSetProcessChunkSize(VFDTPtr vfdt, int value);
void VFDTSetUseGini(VFDTPtr vfdt, int value);
void VFDTSetRestartLeaves(VFDTPtr vfdt, int value);
void VFDTSetCacheTrainingExamples(VFDTPtr vfdt, int value);
void VFDTSetPrePrune(VFDTPtr vfdt, int value);
void VFDTSetAdaptiveDelta(VFDTPtr vfdt, int value);

//void VFDTBootstrapC45(VFDTPtr vfdt, char *fileStem, int overprune, int runC45);
void VFDTProcessExamples(VFDTPtr vfdt, FILE *input);
void VFDTProcessExamplesBatch(VFDTPtr vfdt, FILE *input);
void VFDTProcessExampleBatch(VFDTPtr vfdt, ExamplePtr e);
void VFDTBatchExamplesDone(VFDTPtr vfdt);
void VFDTProcessExample(VFDTPtr vfdt, ExamplePtr e);

int VFDTIsDoneLearning(VFDTPtr vfdt);
long VFDTGetNumGrowing(VFDTPtr vfdt);

void VFDTPrintStats(VFDTPtr vfdt, FILE *out);

/* this will guess on the growing leaves and return a copy of the 
   internally growing decision tree, by processing further examples,
   vfdt can continue to learn */
DecisionTreePtr VFDTGetLearnedTree(VFDTPtr vfdt);


void VFDTREPrune(DecisionTreePtr dt, VoidAListPtr examples);

#endif /* VFDTH */

