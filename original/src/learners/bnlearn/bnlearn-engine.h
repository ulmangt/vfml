#ifndef __BNLEARN_ENGINE_H__
#define __BNLEARN_ENGINE_H__

#include "vfml.h"

/** \ingroup CoreAPI BeliefNet
*/

/** \file 

\brief Learn the structure of a BeliefNet from data 

An API to VFML's Belief Net structure learning engine. The \ref bnlearn
program is basically a wrapper around this interface. You can use this
interface to avoid making system calls to learn belief nets, or the
callbacks in this interface may allow you to modify the learner's
behavior enough so that you don't have to edit any code.

Learns the structure and parameters of a Bayesian network using the
standard method. The structural prior is set by using P(S) = kappa ^
(symetric difference between the net and the prior net). The parameter
prior is K2.

To use this interface you generate a parameters structure using the
BNLearn_NewParams method, fill in any parameters you want (to identify
prior networks, callbacks, training data, etc), then call the BN_Learn
method, extract what you want from the parameters structure, and then
free the structure with BNLearn_FreeParams.  See the header file for
the details of the available parameters.

\thanks to Matthew Richardson for extracting this interface from the
bnlearn learner.

*/

typedef enum BNLAction_ {BNL_NO_CHANGE, BNL_ADDED_PARENT, BNL_REMOVED_PARENT, BNL_REVERSED_PARENT} BNLAction;

// Callback API
// Any of these may be NULL, in which case they are ignored
typedef struct BNLearnCallbackAPI_ {  
  // the user data is one-per-Bn
  // action is what action was taken to create the newBn from the oldBn
  void (*NetInit)(void **user, BeliefNet newBn, BeliefNet oldBn, BNLAction action, int childId, int parentId);
  void (*NetFree)(void **user, BeliefNet bn);
  void (*NodeUpdateBD)(void **user, BeliefNetNode bnn,
       float **add_eventCounts, float *add_rowCounts);
} BNLearnCallbackAPI;


/* The various parameters you may specify to bnlearn, and in
   comments are the default values */
typedef struct BNLearnParams_ {
   // Data Input. One of these should be non-NULL
   VoidListPtr gDataMemory;   // Fastest. Consider using ExamplesRead(...)
   FILE *gDataFile;           // Data is loaded from disk on each iteration. Useful if data does not fit into RAM

   // Input Network. Exactly one of these needs to be non-NULL.
   BeliefNet gInputNetMemory;
   FILE *gInputNetFile;
   ExampleSpecPtr gInputNetEmptySpec; // consider using ExampleSpecRead(char *fileName);

   // Results
   char *gOutputNetFilename;     // name of file to output to. NULL for none
   int gOutputNetToMemory;       // Boolean (defaults to false)
   BeliefNet gOutputNetMemory;   // holds the resulting network upon return

   // Parameters
   long  gLimitBytes;              /*   -1      */
   double gLimitSeconds;           /*   -1      */
   int   gNoReverse;               /*   0       */
   double gKappa;                  /*   0.5     */
   int   gOnlyEstimateParameters;  /*   0       */
   int   gMaxSearchSteps;          /*   -1      */
   int   gMaxParentsPerNode;       /*   -1      */
   int   gMaxParameterGrowthMult;  /*   -1      */
   long  gMaxParameterCount;       /*   -1      */
   int   gSeed;                    /*   -1      */
   double gSmoothAmount;           /*   1.0     */
   int gCheckModelCycle;           /*  0        */
   BNLearnCallbackAPI *gCallbackAPI; /* NULL    */
} BNLearnParams;



/** \brief Makes a new parameters structure with some default values. */
BNLearnParams *BNLearn_NewParams();

/** \brief Frees the parameters structure.

You are responsible for freeing any additonal memory that may have
been allocated by the run (for example any learned BeliefNet).
*/
void BNLearn_FreeParams(BNLearnParams *params);

/** \brief Do the learning. */
void BNLearn(BNLearnParams *params);  


#endif

