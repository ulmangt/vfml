#ifndef VFDTH
#define VFDTH

/** \ingroup CoreAPI DecisionTree
*/

/** \file 

\brief An API which lets your program learn a DecisionTree from a
high-speed data stream.

An API to VFML's engine for learning decision trees from high-speed
data streams.  The \ref vfdt program is basically a wrapper around
this interface. You can use this interface to learn decision trees
from a stream of data as it arrives, and use the tree in a parallel to
make predictions.

To use this interface you generate a VFDTPtr using the VFDTNew function,
set any parameters you want to use to control the learning using the
functions that are described below, and then repeatedly feed examples
to the VFDTPtr using the VFDTProcessExample function.  You can call
VFDTGetLearnedTree at anytime to get a copy of the current learned
tree.  Note that vfdt-engine will take over the memory of any examples
feed to it, and you should not free them or your program will crash.

\wish A function that checkpoints the learning procedure to disk so
that it can be restored at a later time.  I think the hard part of
this would be checkpointing the ExampleGroupStats structure.

*/

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

   int treeLevel;
   float splitConfidence;

   int prePruned;
} VFDTGrowingData, *VFDTGrowingDataPtr;


/** \brief Holds the information needed to learn decision trees from data streams. */
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

   unsigned long numBoundsUsed;

   int cacheExamples;
   int recoverMinimum;

   int reactivateLeaves;
   float highestDeactivatedIndex;
   unsigned long reactivateScanPeriod;

   int batchMode;
   float prePruneTau;
   int laplace;
   int doBonferonni;
} VFDT, *VFDTPtr;

/** \brief Allocate memory to learn a decision tree from a data stream.

Sets up the learning, makes an initial tree with a single node.
splitConfidence is the delta parameter from our paper (the probability
of making a mistake with the sampling we use) and the tieConfidence is
tau (the minimum difference in gain that you care about).
splitConfidence should be a small non-zero number, maybe 10^-7.  And
tieConfidence should be something in the range of 0 - .1 (although
slightly bigger values may be useful).
*/
VFDTPtr VFDTNew(ExampleSpecPtr spec, float splitConfidence,
           float tieConfidence);

/** \brief frees all the memory being used by the learning process. */
void VFDTFree(VFDTPtr vfdt);

/** \brief Higher message levels print more output to the console. 

Levels above 2 print a lot of output.  More than you want.  I promise.
*/
void VFDTSetMessageLevel(VFDTPtr vfdt, int value);

/** \brief Put a limit on the dynamic memory used by the program. 

This requires that DEBUGMEMORY is defined in memory.h (which is the
default).  By setting this you limit the amount of memory allocated
with calls to MemNewPtr by either your program and by vfdt-engine.
This means any other calls you make to VFML functions (e.g. reading
examples from disk) will be counted against vfdt's total.  (you can
use MSetActivePool with pool id 0 to get around this).

If this memory threshold is crossed vfdt-engine starts purging its
allocations by first throwing away cahed examples and then disabling
learning at the least promising leaves.
*/
void VFDTSetMaxAllocationMegs(VFDTPtr vfdt, int value);

/** \brief Sets the number of examples before checks for tree growth.

Check for growth at a leaf once every time it accumulates this many
examples.  Default is 300.
*/
void VFDTSetProcessChunkSize(VFDTPtr vfdt, int value);

/** \brief Set the evaluation function to Gini (default is Entropy) */
void VFDTSetUseGini(VFDTPtr vfdt, int value);

/** \brief Consider reactivating leaves where growing was stopped to save RAM.

The default value for this is true, so periodically vfdt-engine looks
at all the deactivated leaves to see if any of them are more promising
than any of the currently active ones, and makes adjustments.  If set
to false any leaf that is deactivated is effectively pre-pruned.
*/
void VFDTSetRestartLeaves(VFDTPtr vfdt, int value);

/** \brief Use extra RAM to cache training examples.

Default is to cache.  This keeps examples in memory to possibly use
them to help make several decisions, speeding up the induction.  When
RAM is full vfdt-engine starts deactivating example caches at the
least promising leaves.  All caches will be deactivated before any
leaves are deactived.
*/
void VFDTSetCacheTrainingExamples(VFDTPtr vfdt, int value);

/** \brief Set the pre-prune parameter.

The default is 0.0, which means no pre-pruning.  If the gain of all
attributes is less than this value then pre-prune.  Also, do not call
a tie unless an attribute beats another by at least this much.
*/
void VFDTSetPrePruneTau(VFDTPtr vfdt, float value);

/** \brief Set how many examples worth of smoothing to do in class probability estimates. */
void VFDTSetLaplace(VFDTPtr vfdt, int value);

void VFDTSetDoBonferonni(VFDTPtr vfdt, int value);

//void VFDTBootstrapC45(VFDTPtr vfdt, char *fileStem, int overprune, int runC45);

/** \brief Read as many examples as possible from the file and learn from them.

This will repeatedly read and learn from examples in the file until it
can not read any more.  Note that this function will block until that
time.
*/
void VFDTProcessExamples(VFDTPtr vfdt, FILE *input);

/** \brief Learn from the examples in batch mode.

That is, read them all into RAM and use every example to make every
learning decision.
*/
void VFDTProcessExamplesBatch(VFDTPtr vfdt, FILE *input);

/** \brief Add the example to the learner without checking for splits.

When you have added all the examples you want call
VFDTBatchExamplesDone to tell vfdt-engine it is time to make splits.
*/
void VFDTProcessExampleBatch(VFDTPtr vfdt, ExamplePtr e);

/** \brief Forces vfdt-engine to make as many splits as possible using traditional methods. */
void VFDTBatchExamplesDone(VFDTPtr vfdt);

/** \brief Adds another example to the learning process.

Check for splits as needed (according to the chunk size).
*/
void VFDTProcessExample(VFDTPtr vfdt, ExamplePtr e);

/** \brief Returns 1 if no nodes in the tree are still active. 

This may happen because RAM runs out, or because everything was pre-pruned.
*/
int VFDTIsDoneLearning(VFDTPtr vfdt);

/** \brief Returns the number of nodes that are growing */
long VFDTGetNumGrowing(VFDTPtr vfdt);

/** \brief Returns the number of statistical tests made */
long VFDTGetNumBoundsUsed(VFDTPtr vfdt);

/** \brief Prints some information about the growing nodes to the file */
void VFDTPrintStats(VFDTPtr vfdt, FILE *out);



/** \brief Returns the current tree.

This will guess on which class to predict at the growing leaves
(smoothing with strength of the lapace paramterer towards the
distribution seen at the parent).  Returns a copy of the internally
growing decision tree, and so vfdt-engine can
continue to learn on the internal tree unaffected.
*/
DecisionTreePtr VFDTGetLearnedTree(VFDTPtr vfdt);


void VFDTREPrune(DecisionTreePtr dt, VoidAListPtr examples);

#endif /* VFDTH */

