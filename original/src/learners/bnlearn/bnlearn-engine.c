#include "bnlearn-engine.h"
#include <stdio.h>
#include <string.h>

#include <assert.h>

#include <sys/times.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#define BOOL char
#define TRUE 1
#define FALSE 0

long  gInitialParameterCount=0;
long  gBranchFactor=0;
long  gMaxBytesPerModel=0;
BeliefNet gPriorNet=0;


BNLearnParams *BNLearn_NewParams() {
   BNLearnParams *param = (BNLearnParams*)MNewPtr(sizeof(BNLearnParams));

   param->gDataMemory             = NULL;
   param->gDataFile               = NULL;

   param->gInputNetMemory         = NULL;
   param->gInputNetFile           = NULL;
   param->gInputNetEmptySpec      = NULL;

   param->gOutputNetFilename      = NULL;
   param->gOutputNetToMemory      = 0;   
   param->gOutputNetMemory        = NULL;  

   param->gLimitBytes             = -1;
   param->gLimitSeconds           = -1;
   param->gNoReverse              = 0;
   param->gKappa                  = 0.5;
   param->gOnlyEstimateParameters = 0;
   param->gMaxSearchSteps         = -1;
   param->gMaxParentsPerNode      = -1;
   param->gMaxParameterGrowthMult = -1;
   param->gMaxParameterCount      = -1;
   param->gSeed                   = -1;
   param->gSmoothAmount           = 1.0;
   param->gCheckModelCycle        = 0;
   param->gCallbackAPI            = NULL;

   return param;
}

void BNLearn_FreeParams(BNLearnParams *params) {
   MFreePtr(params);
}



BNLearnParams *gParams;


typedef struct BNUserData_ {
   BeliefNetNode changedOne;
   BeliefNetNode changedTwo;

   /* -1 for removal, 0 for nothing, 1 for reverse, 2 for add */
   int changeComplexity;

   // data passed into the callback API, if one exists
   void *callbackAPIdata;   
} BNUserDataStruct, *BNUserData;


typedef struct BNNodeUserData_ {
   double         avgDataLL;
   int            isChangedFromCurrent;
   struct BNNodeUserData_ *current; // null if this is a current node

   // For caching scores:
   double *writebackScore;      //  when you compute the score, write it here
   BOOL *writebackScoreIsValid; //  when you compute the score, set this to valid
   BOOL scoreIsValid; // is avgDataLL already a valid score?

} BNNodeUserDataStruct, *BNNodeUserData;

static void _FreeUserData(BeliefNet bn) {
   int i;
   BeliefNetNode bnn;
   BNUserData netData;

   for(i = 0 ; i < BNGetNumNodes(bn) ; i++) {
      bnn = BNGetNodeByID(bn, i);
      
      MFreePtr(BNNodeGetUserData(bnn));
   }
   netData=BNGetUserData(bn);
   if (gParams->gCallbackAPI && gParams->gCallbackAPI->NetFree)
      gParams->gCallbackAPI->NetFree(&netData->callbackAPIdata, bn);

   MFreePtr(netData);
}



static void _InitUserData(BeliefNet bn, BeliefNet current, BNLAction action, int childId, int parentId) {
   int i;
   BeliefNetNode bnn, currentNode;
   BNNodeUserData data;
   BNUserData netData;

   for(i = 0 ; i < BNGetNumNodes(bn) ; i++) {
      bnn = BNGetNodeByID(bn, i);
      
      data = MNewPtr(sizeof(BNNodeUserDataStruct));
      BNNodeSetUserData(bnn, data);

      data->avgDataLL = 0;
      data->writebackScore=NULL;
      data->writebackScoreIsValid=NULL;
      data->scoreIsValid=FALSE;

      data->isChangedFromCurrent = 0;

      if(bn == current) { /* the init for the current net */
         data->current = 0;
      } else {
         currentNode = BNGetNodeByID(current, i);
         data->current = BNNodeGetUserData(currentNode);
      }
   }

   netData = MNewPtr(sizeof(BNUserDataStruct));
   BNSetUserData(bn, netData);
   netData->changedOne = netData->changedTwo = 0;
   netData->changeComplexity = 0;
   if (gParams->gCallbackAPI && gParams->gCallbackAPI->NetInit)
      gParams->gCallbackAPI->NetInit(&netData->callbackAPIdata, bn, current, action, childId, parentId);
}


static BeliefNetNode _BNGetNodeByID(BeliefNet bn, BeliefNet current, int id) {
   BNUserData netData;

   netData = BNGetUserData(bn);

   if(netData->changedOne) {
      if(BNNodeGetID(netData->changedOne) == id) {
         return netData->changedOne;
      }
   }
   if(netData->changedTwo) {
      if(BNNodeGetID(netData->changedTwo) == id) {
         return netData->changedTwo;
      }
   }

   return BNGetNodeByID(current, id);
}

static int _IsLinkCoveredIn(BeliefNetNode parent, BeliefNetNode child, 
                                                      BeliefNet bn) {
   /* x->y is covered in bn if par(y) = par(x) U x */
   /* this is important for checking equivilance because 
        G and G' are equivilant if identical except for reversal of x->y
                           iff x -> y is covered in G */
   /* NOTE this is k^2 but could be k if the parent lists were kept sorted */

   int i;
   BeliefNetNode childParent;
   int covered = 1;

   if(BNNodeGetNumParents(parent) != BNNodeGetNumParents(child) - 1) {
      covered = 0;
   }

   for(i = 0 ; i < BNNodeGetNumParents(child) && covered ; i++) {
      childParent = BNNodeGetParent(child, i);
      if((parent != childParent) && (!BNNodeHasParent(parent, childParent))) {
         covered = 0;
      }
   }

   return covered;
}



/********************************
  Caching scores for speed
  *********************************/
// Indexed by id of the node, then id of the additional parent.
double **gScoreWithAdditionalParent;
BOOL **gScoreWithAdditionalParentIsValid;
double **gScoreWithRemovedParent;
BOOL **gScoreWithRemovedParentIsValid;
// Indexed by id of the node
double *gScoreAsIs;
BOOL *gScoreAsIsIsValid;

void _InitScoreCache(BeliefNet bn)
{
  int numNodes, i, j;
  numNodes = BNGetNumNodes(bn);
  gScoreWithAdditionalParentIsValid = MNewPtr(sizeof(BOOL*)*numNodes);
  gScoreWithAdditionalParent = MNewPtr(sizeof(double*)*numNodes);
  gScoreWithRemovedParentIsValid = MNewPtr(sizeof(BOOL*)*numNodes);
  gScoreWithRemovedParent = MNewPtr(sizeof(double*)*numNodes);
  gScoreAsIsIsValid = MNewPtr(sizeof(BOOL)*numNodes);
  gScoreAsIs = MNewPtr(sizeof(double)*numNodes);

  for (i=0; i<numNodes; i++) {
    gScoreWithAdditionalParentIsValid[i] = MNewPtr(sizeof(BOOL)*numNodes);
    gScoreWithAdditionalParent[i] = MNewPtr(sizeof(double)*numNodes);
    gScoreWithRemovedParentIsValid[i] = MNewPtr(sizeof(BOOL)*numNodes);
    gScoreWithRemovedParent[i] = MNewPtr(sizeof(double)*numNodes);

    gScoreAsIsIsValid[i]=FALSE;
    for (j=0; j<numNodes; j++) {
      gScoreWithAdditionalParentIsValid[i][j] = FALSE;
      gScoreWithRemovedParentIsValid[i][j] = FALSE;
    }
  }
}


void _InvalidateScoreCache(BeliefNet bn)
{
  BNNodeUserData data;
  BeliefNetNode node;
  int i, j, numNodes;

  numNodes = BNGetNumNodes(bn);
  for (i=0; i<numNodes; i++) {
    node = BNGetNodeByID(bn, i);
    data = BNNodeGetUserData(node);
    if (data->isChangedFromCurrent) {
       DebugMessage(1, 3, "Invalidating cache for node %d\n", i);
       for (j=0; j<numNodes; j++) {
         gScoreWithAdditionalParentIsValid[i][j]=FALSE;
         gScoreWithRemovedParentIsValid[i][j]=FALSE;
       }
       gScoreAsIsIsValid[i] = FALSE;
    }
  }
}


void _SetCachedScore_AddedParent(BNNodeUserData dataOfNewChild, int childId, int parentId) {
  if (gScoreWithAdditionalParentIsValid[childId][parentId]) {
    dataOfNewChild->avgDataLL = gScoreWithAdditionalParent[childId][parentId];
    dataOfNewChild->scoreIsValid = TRUE;
  }
  else
  {
     DebugMessage(1, 3, "need to calc modifying %d by adding %d\n", childId, parentId);
    dataOfNewChild->scoreIsValid = FALSE;
    dataOfNewChild->writebackScore = &(gScoreWithAdditionalParent[childId][parentId]);
    dataOfNewChild->writebackScoreIsValid = &(gScoreWithAdditionalParentIsValid[childId][parentId]);
  }    
}


void _SetCachedScore_RemovedParent(BNNodeUserData dataOfNewChild, int childId, int parentId) {
  if (gScoreWithRemovedParentIsValid[childId][parentId]) {
    dataOfNewChild->avgDataLL = gScoreWithRemovedParent[childId][parentId];
    dataOfNewChild->scoreIsValid = TRUE;
  }
  else
  {
     DebugMessage(1, 3, "need to calc modifying %d by removing %d\n", childId, parentId);
    dataOfNewChild->scoreIsValid = FALSE;
    dataOfNewChild->writebackScore = &(gScoreWithRemovedParent[childId][parentId]);
    dataOfNewChild->writebackScoreIsValid = &(gScoreWithRemovedParentIsValid[childId][parentId]);
  }    
}

void _SetCachedScore_NoParentChange(BNNodeUserData dataOfNewChild, int childId) {
  if (gScoreAsIsIsValid[childId]) {
    dataOfNewChild->avgDataLL = gScoreAsIs[childId];
    dataOfNewChild->scoreIsValid = TRUE;
  }
  else
  {
    DebugMessage(1, 3, "need to calc %d \n", childId);
    dataOfNewChild->scoreIsValid = FALSE;
    dataOfNewChild->writebackScore = &(gScoreAsIs[childId]);
    dataOfNewChild->writebackScoreIsValid = &(gScoreAsIsIsValid[childId]);
  }    
}

/****************************
** End of Caching routines
*****************************/



/* 
  2 possibilities with nodes (a, b):
   no link : make new net for each with a parent link added
   a -> b  : for a make new net with no link for b new net & reverse link

   remember: don't add any BNs with cycles!
   also: don't add any BNs that are equivilent
*/
void _GetOneStepChoicesForBN(BeliefNet bn, VoidListPtr list) {
   int i, j, srcParentOfDstIndex, dstParentOfSrcIndex;
   BeliefNet newBN;
   BeliefNetNode srcNode, dstNode;
   BNNodeUserData data;
   BNUserData netData;
   int isLinkCovered;
   long preAllocation;

   _InitUserData(bn, bn, BNL_NO_CHANGE, 0, 0);

   newBN = BNCloneNoCPTs(bn);
   _InitUserData(newBN, bn, BNL_NO_CHANGE, 0, 0);
   // mattr: The following loop may not be neccessary because any score
   //  checking will look at node->current->AvgDataLL instead of 
   //  node->AvgDataLL.
   for (i=0; i<BNGetNumNodes(bn); i++) {
      _SetCachedScore_NoParentChange( BNNodeGetUserData( BNGetNodeByID(bn, i) ), i );
   }
   VLAppend(list, newBN);

   if(gParams->gOnlyEstimateParameters) {
      return;
   }

   for(i = 0 ; i < BNGetNumNodes(bn) ; i++) {
      for(j = i + 1 ; j < BNGetNumNodes(bn) ; j++) {
         srcNode = BNGetNodeByID(bn, i);
         dstNode = BNGetNodeByID(bn, j);

         dstParentOfSrcIndex = BNNodeLookupParentIndex(srcNode, dstNode);
         srcParentOfDstIndex = BNNodeLookupParentIndex(dstNode, srcNode);

         if(dstParentOfSrcIndex == -1 && srcParentOfDstIndex == -1) {
            /* nodes unrelated, make 2 copies, one with link each way */

            /* copy one */
            preAllocation = MGetTotalAllocation();
            newBN = BNCloneNoCPTs(bn);

            BNFlushStructureCache(newBN);

            srcNode = BNGetNodeByID(newBN, i);
            dstNode = BNGetNodeByID(newBN, j);

            BNNodeAddParent(srcNode, dstNode);
            BNNodeInitCPT(srcNode);

            _InitUserData(newBN, bn, BNL_ADDED_PARENT, i, j);
            data = BNNodeGetUserData(srcNode);
            data->isChangedFromCurrent = 1;
            _SetCachedScore_AddedParent(data, i, j);
            netData = BNGetUserData(newBN);
            netData->changedOne = srcNode;
            netData->changeComplexity = 2;

            isLinkCovered = 0;

            if((gParams->gMaxParentsPerNode == -1 ||
                   BNNodeGetNumParents(srcNode) <= gParams->gMaxParentsPerNode) &&

                   (gParams->gMaxParameterGrowthMult == -1 ||
                    (BNGetNumParameters(newBN) < 
                        (gParams->gMaxParameterGrowthMult * gInitialParameterCount))) &&

                   (gParams->gMaxParameterCount == -1 ||
                    (BNNodeGetNumParameters(srcNode) <= 
                        gParams->gMaxParameterCount)) &&

             (gParams->gLimitBytes == -1 ||
              ((MGetTotalAllocation() - preAllocation) <  gMaxBytesPerModel))&&

                   !BNHasCycle(newBN)) {
               VLAppend(list, newBN);
               isLinkCovered = _IsLinkCoveredIn(dstNode, srcNode, newBN);
            } else {
               _FreeUserData(newBN);
               BNFree(newBN);
            }

            if(!isLinkCovered) {
               /* if it is covered then copy two would be equivilant to
                        copy one, so don't bother adding it */
               /* copy two */
               preAllocation = MGetTotalAllocation();
               newBN = BNCloneNoCPTs(bn);
               BNFlushStructureCache(newBN);

               srcNode = BNGetNodeByID(newBN, i);
               dstNode = BNGetNodeByID(newBN, j);

               BNNodeAddParent(dstNode, srcNode);
               BNNodeInitCPT(dstNode);

               _InitUserData(newBN, bn, BNL_ADDED_PARENT, j, i);
               data = BNNodeGetUserData(dstNode);
               data->isChangedFromCurrent = 1;
               _SetCachedScore_AddedParent(data, j, i);
               netData = BNGetUserData(newBN);
               netData->changedOne = dstNode;
               netData->changeComplexity = 2;

               if((gParams->gMaxParentsPerNode == -1 ||
                      BNNodeGetNumParents(dstNode) <= gParams->gMaxParentsPerNode) &&

                    (gParams->gMaxParameterGrowthMult == -1 ||
                       (BNGetNumParameters(newBN) < 
                        (gParams->gMaxParameterGrowthMult * gInitialParameterCount))) &&

                   (gParams->gMaxParameterCount == -1 ||
                    (BNNodeGetNumParameters(dstNode) <= 
                        gParams->gMaxParameterCount)) &&

             (gParams->gLimitBytes == -1 ||
              ((MGetTotalAllocation() - preAllocation) <  gMaxBytesPerModel))&&

                      !BNHasCycle(newBN)) {
                  VLAppend(list, newBN);
               } else {
                  _FreeUserData(newBN);
                  BNFree(newBN);
               }
            }
         } else {
            /* one is a parent of the other, make 2 copies, one with
                     link removed, one with link reversed */

            /* copy one: remove */
            /* removing a link changes the structure and will not
                        be equivilant to any of the candidates */
            newBN = BNCloneNoCPTs(bn);
            BNFlushStructureCache(newBN);

            preAllocation = MGetTotalAllocation();

            srcNode = BNGetNodeByID(newBN, i);
            dstNode = BNGetNodeByID(newBN, j);

            if(dstParentOfSrcIndex != -1) {
               BNNodeRemoveParent(srcNode, dstParentOfSrcIndex);
               BNNodeInitCPT(srcNode);

               _InitUserData(newBN, bn, BNL_REMOVED_PARENT, i, j);
               data = BNNodeGetUserData(srcNode);
               data->isChangedFromCurrent = 1;
               _SetCachedScore_RemovedParent(data, i, j);
               netData = BNGetUserData(newBN);
               netData->changedOne = srcNode;
               netData->changeComplexity = -1;
            } else {
               BNNodeRemoveParent(dstNode, srcParentOfDstIndex);
               BNNodeInitCPT(dstNode);

               _InitUserData(newBN, bn, BNL_REMOVED_PARENT, j, i);
               data = BNNodeGetUserData(dstNode);
               data->isChangedFromCurrent = 1;
               _SetCachedScore_RemovedParent(data, j, i);
               netData = BNGetUserData(newBN);
               netData->changedOne = dstNode;
               netData->changeComplexity = -1;
            }

            if(!BNHasCycle(newBN) &&

               (gParams->gMaxParameterGrowthMult == -1 ||
                  (BNGetNumParameters(newBN) < 
                  (gParams->gMaxParameterGrowthMult * gInitialParameterCount))) &&

               (gParams->gMaxParameterCount == -1 ||
                  (BNNodeGetNumParameters(srcNode) <= 
                   gParams->gMaxParameterCount  &&
                  BNNodeGetNumParameters(dstNode) <= 
                                         gParams->gMaxParameterCount))&&


               (gParams->gLimitBytes == -1 ||
                    ((MGetTotalAllocation() - preAllocation) <  
                                                gMaxBytesPerModel))) {
               VLAppend(list, newBN);
            } else {
               _FreeUserData(newBN);
               BNFree(newBN);
            }

            /* copy two: reverse */
            if(!gParams->gNoReverse) {
               preAllocation = MGetTotalAllocation();
               newBN = BNCloneNoCPTs(bn);
               BNFlushStructureCache(newBN);

               srcNode = BNGetNodeByID(newBN, i);
               dstNode = BNGetNodeByID(newBN, j);

               if(dstParentOfSrcIndex != -1) {
                  BNNodeRemoveParent(srcNode, dstParentOfSrcIndex);
                  BNNodeAddParent(dstNode, srcNode);
                  BNNodeInitCPT(srcNode);
                  BNNodeInitCPT(dstNode);

                  _InitUserData(newBN, bn, BNL_REVERSED_PARENT, i, j);
                  data = BNNodeGetUserData(srcNode);
                  data->isChangedFromCurrent = 1;
                  _SetCachedScore_RemovedParent(data, i, j);                  
                  data = BNNodeGetUserData(dstNode);
                  data->isChangedFromCurrent = 1;
                  _SetCachedScore_AddedParent(data, j, i);  
                  netData = BNGetUserData(newBN);
                  netData->changedOne = srcNode;
                  netData->changedTwo = dstNode;
                  netData->changeComplexity = 1;

                  isLinkCovered = _IsLinkCoveredIn(srcNode, dstNode, newBN);
               } else {
                  BNNodeRemoveParent(dstNode, srcParentOfDstIndex);
                  BNNodeAddParent(srcNode, dstNode);
                  BNNodeInitCPT(srcNode);
                  BNNodeInitCPT(dstNode);

                  _InitUserData(newBN, bn, BNL_REVERSED_PARENT, j, i);
                  data = BNNodeGetUserData(srcNode);
                  data->isChangedFromCurrent = 1;
                  _SetCachedScore_AddedParent(data, i, j);

                  data = BNNodeGetUserData(dstNode);
                  data->isChangedFromCurrent = 1;
                  _SetCachedScore_RemovedParent(data, j, i);
                  netData = BNGetUserData(newBN);
                  netData->changedOne = srcNode;
                  netData->changedTwo = dstNode;
                  netData->changeComplexity = 1;

                  isLinkCovered = _IsLinkCoveredIn(dstNode, srcNode, newBN);
               }

               if((gParams->gMaxParentsPerNode == -1 ||
                      (BNNodeGetNumParents(srcNode) <= gParams->gMaxParentsPerNode &&
                       BNNodeGetNumParents(dstNode) <= gParams->gMaxParentsPerNode)) &&

                !isLinkCovered &&

             (gParams->gLimitBytes == -1 ||
              ((MGetTotalAllocation() - preAllocation) <  gMaxBytesPerModel))&&

                (gParams->gMaxParameterGrowthMult == -1 ||
                    (BNGetNumParameters(newBN) < 
                        (gParams->gMaxParameterGrowthMult * gInitialParameterCount))) &&

               (gParams->gMaxParameterCount == -1 ||
                  (BNNodeGetNumParameters(srcNode) <= 
                   gParams->gMaxParameterCount  &&
                  BNNodeGetNumParameters(dstNode) <= 
                                         gParams->gMaxParameterCount))&&

                !BNHasCycle(newBN)) {
                  VLAppend(list, newBN);
               } else {
                  _FreeUserData(newBN);
                  BNFree(newBN);
               }
            }
         }
      }
   }
}

static void _OptimizedAddSampleInit(BeliefNet current, VoidListPtr newNets) {
   BNNodeUserData nodeData;
   int i;

   for(i=0; i<VLLength(current->nodes); i++) {
      BeliefNetNode node = (BeliefNetNode)VLIndex(current->nodes, i);
      nodeData = BNNodeGetUserData(node);
      if (!nodeData->scoreIsValid) {
         BNNodeZeroCPT(node);
      }
   }

   for(i = 0 ; i < VLLength(newNets) ; i++) {
      BNUserData netData = BNGetUserData(VLIndex(newNets, i));

      if(netData->changedOne) {
         nodeData = BNNodeGetUserData(netData->changedOne);
         if (!nodeData->scoreIsValid) {
            BNNodeZeroCPT(netData->changedOne);
         }
      }
      if(netData->changedTwo) {
         nodeData = BNNodeGetUserData(netData->changedTwo);
         if (!nodeData->scoreIsValid) {
            BNNodeZeroCPT(netData->changedTwo);
         }
      }
   }
}


static void _OptimizedAddSample(BeliefNet current, VoidListPtr newNets, 
                                               ExamplePtr e) {
   BNUserData netData;
   BNNodeUserData nodeData;
   int i;

   //BNAddSample(current, e);
   for(i=0; i<VLLength(current->nodes); i++) {
      BeliefNetNode node = (BeliefNetNode)VLIndex(current->nodes, i);
      nodeData = BNNodeGetUserData(node);
      if (!nodeData->scoreIsValid) {
         BNNodeAddSample(node, e);
      }
   }

   for(i = 0 ; i < VLLength(newNets) ; i++) {
      netData = BNGetUserData(VLIndex(newNets, i));

      if(netData->changedOne) {
         nodeData = BNNodeGetUserData(netData->changedOne);
         if (!nodeData->scoreIsValid)
            BNNodeAddSample(netData->changedOne, e);
      }
      if(netData->changedTwo) {
         nodeData = BNNodeGetUserData(netData->changedTwo);
         if (!nodeData->scoreIsValid)
            BNNodeAddSample(netData->changedTwo, e);
      }
   }
}

static void _OptimizedAddSamples(BeliefNet current, VoidListPtr newNets, 
                                               VoidListPtr samples) {
   BNUserData netData;
   BNNodeUserData nodeData;
   int i;

   //BNAddSamples(current, samples);
   for(i=0; i<VLLength(current->nodes); i++) {
      BeliefNetNode node = (BeliefNetNode)VLIndex(current->nodes, i);
      nodeData = BNNodeGetUserData(node);
      if (!nodeData->scoreIsValid) {
         BNNodeZeroCPT(node);
         BNNodeAddSamples(node, samples);
      }
   }

   for(i = 0 ; i < VLLength(newNets) ; i++) {
      netData = BNGetUserData(VLIndex(newNets, i));

      if(netData->changedOne) {
         nodeData = BNNodeGetUserData(netData->changedOne);
         if (!nodeData->scoreIsValid) {
            BNNodeZeroCPT(netData->changedOne);
            BNNodeAddSamples(netData->changedOne, samples); 
         }
      }
      if(netData->changedTwo) {
         nodeData = BNNodeGetUserData(netData->changedTwo);
         if (!nodeData->scoreIsValid) {
            BNNodeZeroCPT(netData->changedTwo);
            BNNodeAddSamples(netData->changedTwo, samples); 
         }
      }
   }
}

static float _ScoreBN(BeliefNet bn) {
   int numCPTRows;
   int i,j,k;
   BeliefNetNode bnn;
   double score;
   double prob, numSamples;

   /* score is sum over atribs, over parent combos, over attrib value of:
         p_ijk lg P_ijk - p_ij lg p_ij */


   score = 0;

   for(i = 0 ; i < BNGetNumNodes(bn) ; i++) {
      bnn = BNGetNodeByID(bn, i);

      numSamples = BNNodeGetNumSamples(bnn);
      numCPTRows = BNNodeGetNumCPTRows(bnn);

      for(j = 0 ; j < numCPTRows ; j++) {
         /* HACK for efficiency break BNN ADT */
         for(k = 0 ; k < BNNodeGetNumValues(bnn) ; k++) {
            if(numSamples != 0) {
               prob = bnn->eventCounts[j][k] / numSamples;
               DebugMessage(1, 4,
                 "   i: %d j: %d k: %d eventcount: %lf rowcount: %lf\n",
                     i, j, k, bnn->eventCounts[j][k],
                       bnn->rowCounts[j]);
               if(prob != 0) {
                  score += prob * log(prob);
                  DebugMessage(1, 4, "   Score now: %lf\n", score);
               }
            }
         }
         if(numSamples != 0) {
            prob = bnn->rowCounts[j] / BNNodeGetNumSamples(bnn);
            DebugMessage(1, 4,
              "   i: %d j: %d rowcount: %lf numsamples: %lf\n",
                  i, j, bnn->rowCounts[j], 
                            numSamples);
            if(prob != 0) {
               score -= prob * log(prob);
               DebugMessage(1, 4, "   Score now: %lf\n", score);
            }
         }
      }
   }

   return score;
}

static double _GetStructuralDifferenceScoreNode(BeliefNetNode bnn) {
   int difference = 0;
   int i;
   BeliefNetNode priorNode;

   priorNode = BNGetNodeByID(gPriorNet, BNNodeGetID(bnn));
   for(i = 0 ; i < BNNodeGetNumParents(bnn) ; i++) {
      if(!BNNodeHasParentID(priorNode, BNNodeGetParentID(bnn, i))) {
         difference++;
      }
   }
   for(i = 0 ; i < BNNodeGetNumParents(priorNode) ; i++) {
      if(!BNNodeHasParentID(bnn, BNNodeGetParentID(priorNode, i))) {
         difference++;
      }
   }

   return difference * log(gParams->gKappa);
}


void _UpdateNodeBD(BeliefNetNode bnn) {
   int numCPTRows, numValues;
   BNNodeUserData data;
   int j, k;
   double numSamples;
   double gamma_numValues, gamma_1;

   data = BNNodeGetUserData(bnn);
   if (data->scoreIsValid) {
     //printf("cached value is %g\n", data->avgDataLL);
     return;
   }

   if (gParams->gCallbackAPI && gParams->gCallbackAPI->NodeUpdateBD) {
      BNUserData netData = BNGetUserData(bnn->bn);
      gParams->gCallbackAPI->NodeUpdateBD(&netData->callbackAPIdata, bnn, bnn->eventCounts, bnn->rowCounts);
   }

   data->avgDataLL = 0;
   numCPTRows = BNNodeGetNumCPTRows(bnn);
   numSamples = BNNodeGetNumSamples(bnn);
   numValues = BNNodeGetNumValues(bnn);     

   gamma_numValues = StatLogGamma(numValues);
   gamma_1 = StatLogGamma(1);
   for(j = 0 ; j < numCPTRows ; j++) {
      /* HACK for efficiency break BNN ADT */

      /* HERE HERE update this to use the probabilities from the
               prior network */
      data->avgDataLL += gamma_numValues -
         StatLogGamma(numValues + bnn->rowCounts[j]);
      for(k = 0 ; k < numValues ; k++) {
         data->avgDataLL += StatLogGamma(1 + bnn->eventCounts[j][k]);
      }
      data->avgDataLL -= numValues*gamma_1;
   }
   /* now scale it by the likelihood of the node given structural prior */
   data->avgDataLL += _GetStructuralDifferenceScoreNode(bnn);

   if (data->writebackScore) {
     *(data->writebackScore) = data->avgDataLL;
     *(data->writebackScoreIsValid) = TRUE;
   }



}

static void _UpdateCurrentNetScoreBD(BeliefNet bn) {
   int i;

   for(i = 0 ; i < BNGetNumNodes(bn) ; i++) {
      _UpdateNodeBD(BNGetNodeByID(bn, i));
   }
}

static void _UpdateNetScoreBD(BeliefNet bn) {
   BNUserData netData = BNGetUserData(bn);

   if(netData->changedOne) {
      _UpdateNodeBD(netData->changedOne);
   }
   if(netData->changedTwo) {
      _UpdateNodeBD(netData->changedTwo);
   }
}


static double _GetNodeScore(BeliefNetNode bnn) {
   BNNodeUserData data = BNNodeGetUserData(bnn);
   
   if(data->current == 0 || data->isChangedFromCurrent) {
      return data->avgDataLL;
   } else {
      return data->current->avgDataLL;
   }
}

float _ScoreBNOptimized(BeliefNet bn);

static int _IsFirstNetBetter(BeliefNet first, BeliefNet second) {
   BNUserData firstData = BNGetUserData(first);
   BNUserData secondData = BNGetUserData(second);
   int i1, i2, i;
   double scoreOne, scoreTwo;
   
   scoreOne = scoreTwo = 0;
   i1 = i2 = -1;

   if(firstData->changedOne) {
      i = i1 = BNNodeGetID(firstData->changedOne);
      scoreOne += _GetNodeScore(BNGetNodeByID(first, i));
      scoreTwo += _GetNodeScore(BNGetNodeByID(second, i));
   }

   if(firstData->changedTwo) {
      i = i2 = BNNodeGetID(firstData->changedTwo);
      scoreOne += _GetNodeScore(BNGetNodeByID(first, i));
      scoreTwo += _GetNodeScore(BNGetNodeByID(second, i));
   }

   if(secondData->changedOne) {
      i = BNNodeGetID(secondData->changedOne);
      if(i != i1 && i != i2) {
         /* don't compare same nodes twice */
         scoreOne += _GetNodeScore(BNGetNodeByID(first, i));
         scoreTwo += _GetNodeScore(BNGetNodeByID(second, i));
      }
   }

   if(secondData->changedTwo) {
      i = BNNodeGetID(secondData->changedTwo);
      if(i != i1 && i != i2) {
         /* don't compare same nodes twice */
         scoreOne += _GetNodeScore(BNGetNodeByID(first, i));
         scoreTwo += _GetNodeScore(BNGetNodeByID(second, i));
      }
   }

   return scoreOne > scoreTwo;
}

float _ScoreBNOptimized(BeliefNet bn) {
   int i;
   double score;

   score = 0;

   for(i = 0 ; i < BNGetNumNodes(bn) ; i++) {
      score += _GetNodeScore(BNGetNodeByID(bn, i));
   }

   return score;
}

static void _UpdateCPTsForFrom(BeliefNet target, BeliefNet source) {
   BeliefNetNode targetNode, sourceNode;
   BNNodeUserData data;
   int i;

   for(i = 0 ; i < BNGetNumNodes(target) ; i++) {
      targetNode = BNGetNodeByID(target, i);
      sourceNode = BNGetNodeByID(source, i);
      data = BNNodeGetUserData(targetNode);

      if(!data->isChangedFromCurrent) {
         BNNodeInitCPT(targetNode);
         BNNodeSetCPTFrom(targetNode, sourceNode);
      }
   }
}

static int _IsCurrentNet(BeliefNet bn) {
   BNUserData data;

   data = (BNUserData)BNGetUserData(bn);

   return (data->changedOne == 0) && (data->changedTwo == 0);
}

static void _CompareNetsFreeLoosers(BeliefNet current, 
                                           VoidListPtr netChoices) {
   BeliefNet netOne, netTwo;

   _UpdateCurrentNetScoreBD(current);

   netOne = VLRemove(netChoices, VLLength(netChoices) - 1);
   _UpdateNetScoreBD(netOne);
   while(VLLength(netChoices)) {
      netTwo = VLRemove(netChoices, VLLength(netChoices) - 1);
      _UpdateNetScoreBD(netTwo);
      if(_IsFirstNetBetter(netOne, netTwo)) {
         _FreeUserData(netTwo);
         BNFree(netTwo);
      } else {
         _FreeUserData(netOne);
         BNFree(netOne);
         netOne = netTwo;
      }
   }

   VLAppend(netChoices, netOne);
}

void _AllocFailed(int allocationSize) {
   printf("MEMORY ALLOCATION FAILED, size %d\n", allocationSize);
}

static int _IsTimeExpired(struct tms starttime) {
   struct tms endtime;
   long seconds;

   if(gParams->gLimitSeconds != -1) {
      times(&endtime);
      seconds = (double)(endtime.tms_utime - starttime.tms_utime) / 100.0;

      return seconds >= gParams->gLimitSeconds;
   }

   return 0;
}


void BNLearn(BNLearnParams *params) {
   ExampleSpecPtr es;
   int allDone, searchStep;
   BeliefNet bn;

   long learnTime, allocation, seenTotal;
   struct tms starttime;
   struct tms endtime;

   VoidListPtr netChoices;
   VoidListPtr previousWinners;

   gParams = params;
   previousWinners = VLNew();

   MSetAllocFailFunction(_AllocFailed);   

   // Set up the input network
   if (gParams->gInputNetMemory) {
      bn = gParams->gInputNetMemory;      
   }
   else if(gParams->gInputNetFile) {
      bn = BNReadBIFFILEP(gParams->gInputNetFile);
      if(bn == 0) {
         DebugMessage(1, 1, "couldn't read net\n");
      }
      BNZeroCPTs(bn);
   }
   else if(gParams->gInputNetEmptySpec) {
      bn = BNNewFromSpec(gParams->gInputNetEmptySpec);
   }
   else {
      DebugError(1,"Error, at least one of gInputNetMemory, gInputNetFile, or gInputNetEmptySpec must be specified");
      return;
   }
   assert(bn);
   es = BNGetExampleSpec(bn);

   if (!gParams->gDataMemory && !gParams->gDataFile) {
      DebugError(1,"Error, at least one of gDataMemory or gDataFile must be set");
      return;
   }

   gInitialParameterCount = BNGetNumParameters(bn);
   gBranchFactor = BNGetNumNodes(bn) * BNGetNumNodes(bn);
   if(gParams->gLimitBytes != -1) {
      gMaxBytesPerModel = gParams->gLimitBytes / gBranchFactor;
      DebugMessage(1, 2, "Limit models to %.4lf megs\n", 
                     gMaxBytesPerModel / (1024.0 * 1024.0));
   }

   gPriorNet = BNClone(bn);

   RandomInit();
   /* seed */
   if(gParams->gSeed != -1) {
      RandomSeed(gParams->gSeed);
   } else {
      gParams->gSeed = RandomRange(1, 30000);
      RandomSeed(gParams->gSeed);
   }

   DebugMessage(1, 1, "running with seed %d\n", gParams->gSeed);
   DebugMessage(1, 1, "allocation %ld\n", MGetTotalAllocation());
   DebugMessage(1, 1, "initial parameters %ld\n", gInitialParameterCount);

   times(&starttime);

   seenTotal = 0;
   learnTime = 0;

   _InitScoreCache(bn);
   allDone = 0;
   searchStep = 0;
   while(!allDone) {
      searchStep++;
      DebugMessage(1, 2, "============== Search step: %d ==============\n",
             searchStep);
      DebugMessage(1, 2, "  Total samples: %ld\n", seenTotal);
      DebugMessage(1, 2, " allocation before choices %ld\n", 
                                               MGetTotalAllocation());

      DebugMessage(1, 2, "   best with score %f:\n", _ScoreBN(bn));
      if(DebugGetMessageLevel() >= 2) {
         BNPrintStats(bn);
      }

      if(DebugGetMessageLevel() >= 3) {
         BNWriteBIF(bn, DebugGetTarget());
      }

      netChoices = VLNew();

      _GetOneStepChoicesForBN(bn, netChoices);

      DebugMessage(1, 2, " allocation after choices %ld there are %d\n", 
                         MGetTotalAllocation(), VLLength(netChoices));

      if (gParams->gDataMemory) {
         _OptimizedAddSamples(bn, netChoices, gParams->gDataMemory);
         seenTotal += VLLength(gParams->gDataMemory);
         if(_IsTimeExpired(starttime))
            allDone = 1;      
      }
      if (gParams->gDataFile) {
         
         int stepDone=0;
         ExamplePtr e = ExampleRead(gParams->gDataFile, es);
         _OptimizedAddSampleInit(bn, netChoices);    
         while(!stepDone && e != 0) {
            seenTotal++;
            /* put the eg in every net choice */
            _OptimizedAddSample(bn, netChoices, e);

            if(_IsTimeExpired(starttime)) {
               stepDone = allDone = 1;
            }
            ExampleFree(e);
            e = ExampleRead(gParams->gDataFile, es);
         } /* !stepDone && e != 0 */
      }


      _CompareNetsFreeLoosers(bn, netChoices);

      /* if the winner is the current one then we are all done */
      if(BNStructureEqual(bn, (BeliefNet)VLIndex(netChoices, 0)) ||
         !_IsFirstNetBetter((BeliefNet)VLIndex(netChoices,0), bn)) {
         /* make sure to free all loosing choices and the netChoices list */
         allDone = 1;
      } else if(gParams->gMaxSearchSteps != -1 && searchStep >= gParams->gMaxSearchSteps) {
         DebugMessage(1, 1, "Stopped because of search step limit\n");
         allDone = 1;
      }

      /* copy all the CPTs that are only in bn into the new winner */
       /* only really needed for final output but I do it for debugging too */
      _UpdateCPTsForFrom((BeliefNet)VLIndex(netChoices, 0), bn);
      _InvalidateScoreCache((BeliefNet)VLIndex(netChoices, 0));   

      if(gParams->gOnlyEstimateParameters) {
         allDone = 1;
      }

      if(gParams->gCheckModelCycle) {
         /* now check all previous winners */
         /* if we detect a cycle, pick the best that happens
               in the period of the cycle */
         if(!allDone) {
            int i;
            for(i = 0 ; i < VLLength(previousWinners) ; i++) {
               if(BNStructureEqual(bn, (BeliefNet)VLIndex(previousWinners, i))) {
                  allDone = 1;
               }

               if(allDone) {
                  if(_ScoreBN(bn) < 
                          _ScoreBN((BeliefNet)VLIndex(previousWinners, i))) {
                     bn = VLIndex(previousWinners, i);
                  }
               }
            }
         }
         VLAppend(previousWinners, BNClone(bn));
      }

      _FreeUserData(bn);
      BNFree(bn);
      bn = (BeliefNet)VLRemove(netChoices, 0);
      _FreeUserData(bn);
      VLFree(netChoices);

      DebugMessage(1, 2, " allocation after all free %ld\n", 
                                           MGetTotalAllocation());

      /* reset data file */
      if (gParams->gDataFile)
         rewind(gParams->gDataFile);

   } /* while !allDone */

   if (gParams->gSmoothAmount != 0) {
     BNSmoothProbabilities(bn, gParams->gSmoothAmount);
   }

   times(&endtime);
   learnTime += endtime.tms_utime - starttime.tms_utime;

   DebugMessage(1, 1, "done learning...\n");
   DebugMessage(1, 1, "time %.2lfs\n", ((double)learnTime) / 100);

   DebugMessage(1, 1, "Total Samples: %ld\n", seenTotal);
   if(DebugGetMessageLevel() >= 1) {
      DebugMessage(1, 1, "Samples per round:\n");
   }

   allocation = MGetTotalAllocation();

   //printf("Final score: %f\n", _ScoreBN(bn));
   
   if (gParams->gOutputNetFilename) {
      FILE *netOut = fopen(gParams->gOutputNetFilename, "w");
      BNWriteBIF(bn, netOut);
      fclose(netOut);
   }

   if (gParams->gOutputNetToMemory) {
      gParams->gOutputNetMemory = bn;
   }
   else
      BNFree(bn);

   //ExampleSpecFree(es);
   DebugMessage(1, 1, "   allocation %ld\n", MGetTotalAllocation());
}
