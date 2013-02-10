#include "ModelSearch.h"

#include <math.h>

/* lame hack globals */
extern int   gMaxParentsPerNode;
extern long  gMaxParameterCount;
extern long  gMaxBytesPerModel;
extern int   gLimitBytes;
extern float gDelta;
extern int   gP0Multiplier;
extern long  gNumBoundsUsed;
extern float gTau;
extern int   gAllowRemove;

extern long  gNumByTie;
extern long  gNumByWin;
extern long  gMinSamplesInDecision;
extern long  gMaxSamplesInDecision;
extern long  gTotalSamplesInDecision;
extern long  gNumZeroSamplesInDecision;
extern long  gNumCurrentInTie;
extern long  gNumCurrentByWin;
extern long  gNumCurrentByDefault;
extern long  gNumByCycleConflict;
extern long  gNumByParameterConflict;
extern long  gNumByParentLimit;
extern long  gNumByMemoryLimit;
extern long  gNumByParameterLimit;
extern long  gNumByChangeLimit;
extern long  gNumRemoved;
extern long  gNumAdded;
extern long  gNumUsedWholeDB;

typedef struct BNUserData_ {
   BeliefNetNode changedOne;
   int changedParentID;

   /* -1 for removal, 0 for nothing, 1 for reverse, 2 for add */
   int changeComplexity;

   BeliefNet currentNet;
} BNUserDataStruct, *BNUserData;


typedef struct BNNodeUserData_ {
   double         avgDataLL;
//   double         score;
//   double         upperBound;
//   double         lowerBound;
   double         p0;
   int            isChangedFromCurrent;
   struct BNNodeUserData_ *current; // null if this is a current node
} BNNodeUserDataStruct, *BNNodeUserData;

static void _FreeUserData(BNUserData netData) {

   if(netData->changedOne) {
      MFreePtr(BNNodeGetUserData(netData->changedOne));
      BNNodeFree(netData->changedOne);
   }

   MFreePtr(netData);
}


int _BNHasCycle(BNUserData netData) {
   //BeliefNet newBN;
   BeliefNetNode changedNode, parentNode;
   int hasCycle;

   if(netData->changeComplexity == -1 || netData->changeComplexity == 0) {
      hasCycle = BNHasCycle(netData->currentNet);
   } else {
      changedNode = BNGetNodeByID(netData->currentNet,
                                   BNNodeGetID(netData->changedOne));
      parentNode = BNGetNodeByID(netData->currentNet,
                                          netData->changedParentID);

      if(BNNodeGetNumChildren(changedNode) == 0 || 
             BNNodeGetNumParents(parentNode) == 0) {
         hasCycle = 0;
      } else {
         //newBN = BNCloneNoCPTs(netData->currentNet);

        //changedNode = BNGetNodeByID(newBN, BNNodeGetID(netData->changedOne));
        //parentNode = BNGetNodeByID(newBN, netData->changedParentID);

         if(netData->changeComplexity == 2) {
            BNFlushStructureCache(netData->currentNet);
            BNNodeAddParent(changedNode, parentNode);
            hasCycle = BNHasCycle(netData->currentNet);
            BNNodeRemoveParent(changedNode, 
                       BNNodeLookupParentIndex(changedNode, parentNode));
            BNFlushStructureCache(netData->currentNet);
         } else {
            /* must be a remove, but that is special cased elsewhere
                    and can't cause a cycle */
            //BNNodeRemoveParent(changedNode, 
            //     BNNodeLookupParentIndex(changedNode, parentNode));
            hasCycle = 0;
         }
         //hasCycle = BNHasCycle(newBN);
         //BNFree(newBN);
      }
   }

   return hasCycle;
}

BeliefNetNode _BNNodeCloneNoCPT(BeliefNetNode bnn, BeliefNet newBN);
static BNUserData _InitUserData(int nodeID, int doChange, int changedParentID,
                                 int isAdd, BeliefNet current) {
   BNNodeUserData data;
   BNUserData netData;
   BeliefNetNode changedBnn, currentBnn;
   int changeComplexity = 0;

   if(doChange) {
      changedBnn = _BNNodeCloneNoCPT(BNGetNodeByID(current, nodeID),
                                                current);
      data = MNewPtr(sizeof(BNNodeUserDataStruct));
      BNNodeSetUserData(changedBnn, data);

      data->avgDataLL = 0;

      data->isChangedFromCurrent = 1;
      data->p0 = 1;

      currentBnn = BNGetNodeByID(current, nodeID);
      data->current = BNNodeGetUserData(currentBnn);

      if(isAdd) {
         changeComplexity = 2;
         /* HERE break BN ADT right here */
         VLAppend(changedBnn->parentIDs, (void *)changedParentID);
         changedBnn->numParentCombinations = -1;
         BNNodeInitCPT(changedBnn);
      } else {
         /* remove */
         changeComplexity = -1;
         /* HERE break BN ADT right here */
         VLRemove(changedBnn->parentIDs, 
                     BNNodeLookupParentIndexByID(changedBnn, changedParentID));
         changedBnn->numParentCombinations = -1;
         BNNodeInitCPT(changedBnn);
      }

      /* HERE HERE Heuristic hack */
      //BNNodeSmoothProbabilities(changedBnn, 1.0 / 
      //                      (float)BNNodeGetNumValues(changedBnn));
   } else {
      changedBnn = 0;
   }

   netData = MNewPtr(sizeof(BNUserDataStruct));
   netData->changedOne = changedBnn;
   netData->changedParentID = changedParentID;
   netData->changeComplexity = changeComplexity;
   netData->currentNet = current;

   return netData;
}

static void _InitCurrentUserData(BeliefNet current) {
   int i;
   BeliefNetNode bnn;
   BNNodeUserData data;

   if(BNGetUserData(current) == 0) {
      for(i = 0 ; i < BNGetNumNodes(current) ; i++) {
         bnn = BNGetNodeByID(current, i);
      
         data = MNewPtr(sizeof(BNNodeUserDataStruct));
         BNNodeSetUserData(bnn, data);

         data->avgDataLL = 0;

         data->isChangedFromCurrent = 0;
         data->p0 = 1;

         data->current = 0;
      }

      /* flag to keep it from getting inited too many times */
      BNSetUserData(current, (void *)1);
   }
}

static BeliefNetNode _BNGetNodeByID(BNUserData netData, int id) {
   if(netData->changedOne) {
      if(BNNodeGetID(netData->changedOne) == id) {
         return netData->changedOne;
      }
   }

   return BNGetNodeByID(netData->currentNet, id);
}

static double _CalculateCP(BeliefNetNode bnn, double event, double row) {
   /* HERE HERE Heuristic hack */
   return (event + 1.0) / (row + (float)BNNodeGetNumValues(bnn));
   //return event / row;
}

static double _GetEpsilonNormal(BNUserData firstData, BNUserData secondData,
                                                       BeliefNet current) {
   BeliefNetNode bnn;
   int i, j, k, numCPTRows, n1;
   double likelihood, numSamples;
   double bound;
   StatTracker st;
   
   bound = 0;

   //n1 = -1;
   if(firstData->changedOne) {
      bnn = firstData->changedOne;
      n1 = BNNodeGetID(bnn);
      st = StatTrackerNew();

      numCPTRows = BNNodeGetNumCPTRows(bnn);
      numSamples = BNNodeGetNumSamples(bnn);
      for(j = 0 ; j < numCPTRows ; j++) {
         /* HACK for efficiency break BNN ADT */
         for(k = 0 ; k < BNNodeGetNumValues(bnn) ; k++) {
            likelihood = (bnn->eventCounts[j][k] / numSamples) * 
               log(_CalculateCP(bnn, bnn->eventCounts[j][k],
                                     bnn->rowCounts[j]));
            for(i = 0 ; i < bnn->eventCounts[j][k] ; i++) {
               StatTrackerAddSample(st, likelihood);
            }
         }
      }

      bound += StatTrackerGetNormalBound(st, gDelta);
      StatTrackerFree(st);

      bnn = _BNGetNodeByID(secondData, n1);
      st = StatTrackerNew();

      numCPTRows = BNNodeGetNumCPTRows(bnn);
      numSamples = BNNodeGetNumSamples(bnn);
      for(j = 0 ; j < numCPTRows ; j++) {
         /* HACK for efficiency break BNN ADT */
         for(k = 0 ; k < BNNodeGetNumValues(bnn) ; k++) {
            likelihood = (bnn->eventCounts[j][k] / numSamples) * 
               log(_CalculateCP(bnn, bnn->eventCounts[j][k],
                                     bnn->rowCounts[j]));
            for(i = 0 ; i < bnn->eventCounts[j][k] ; i++) {
               StatTrackerAddSample(st, likelihood);
            }
         }
      }

      bound += StatTrackerGetNormalBound(st, gDelta);
      StatTrackerFree(st);
   } else if(secondData->changedOne) {
      bnn = secondData->changedOne;
      n1 = BNNodeGetID(bnn);
      st = StatTrackerNew();

      numCPTRows = BNNodeGetNumCPTRows(bnn);
      numSamples = BNNodeGetNumSamples(bnn);
      for(j = 0 ; j < numCPTRows ; j++) {
         /* HACK for efficiency break BNN ADT */
         for(k = 0 ; k < BNNodeGetNumValues(bnn) ; k++) {
            likelihood = (bnn->eventCounts[j][k] / numSamples) * 
               log(_CalculateCP(bnn, bnn->eventCounts[j][k],
                                     bnn->rowCounts[j]));
            for(i = 0 ; i < bnn->eventCounts[j][k] ; i++) {
               StatTrackerAddSample(st, likelihood);
            }
         }
      }

      bound += StatTrackerGetNormalBound(st, gDelta);
      StatTrackerFree(st);

      bnn = _BNGetNodeByID(firstData, n1);
      st = StatTrackerNew();

      numCPTRows = BNNodeGetNumCPTRows(bnn);
      numSamples = BNNodeGetNumSamples(bnn);
      for(j = 0 ; j < numCPTRows ; j++) {
         /* HACK for efficiency break BNN ADT */
         for(k = 0 ; k < BNNodeGetNumValues(bnn) ; k++) {
            likelihood = (bnn->eventCounts[j][k] / numSamples) * 
               log(_CalculateCP(bnn, bnn->eventCounts[j][k],
                                     bnn->rowCounts[j]));
            for(i = 0 ; i < bnn->eventCounts[j][k] ; i++) {
               StatTrackerAddSample(st, likelihood);
            }
         }
      }

      bound += StatTrackerGetNormalBound(st, gDelta);
      StatTrackerFree(st);
   }

   return bound;
}


static double _GetNodeScoreRange(BeliefNetNode bnn) {
   double p0;

   BNNodeUserData data = BNNodeGetUserData(bnn);

   if(data == 0) {
      p0 = (1.0 / (5.0 * (double)BNNodeGetNumValues(bnn)));
   } else {
      p0 = min(1.0 / (5.0 * (double)BNNodeGetNumValues(bnn)), 
               data->p0 / (double)gP0Multiplier);
   }

   return fabs(log(p0));
}

static double _GetComparedNodesScoreRange(BNUserData firstData,
                                              BNUserData secondData) {
   double scoreRange;

  
   scoreRange = 0;

   if(firstData->changedOne) {
      scoreRange += _GetNodeScoreRange(firstData->changedOne);
   } else if(secondData->changedOne) {
      scoreRange += _GetNodeScoreRange(secondData->changedOne);
   }

   return scoreRange;
}

void _UpdateNodeAveDataLL(BeliefNetNode bnn) {
   int numCPTRows;
   BNNodeUserData data;
   int j, k;
   double numSamples;
   double prob, logCP;

   data = BNNodeGetUserData(bnn);

   /* if this is the current node or is changed from it */
   if(data->isChangedFromCurrent || (data->current == 0)) {
      data->avgDataLL = 0;
      numCPTRows = BNNodeGetNumCPTRows(bnn);
      numSamples = BNNodeGetNumSamples(bnn);

      for(j = 0 ; j < numCPTRows ; j++) {
         /* HACK for efficiency break BNN ADT */
         for(k = 0 ; k < BNNodeGetNumValues(bnn) ; k++) {
            if(bnn->eventCounts[j][k] > 0) {
               prob = bnn->eventCounts[j][k] / numSamples;

               /* HERE HERE Heuristic Hack */
               //logCP = log(bnn->eventCounts[j][k] / bnn->rowCounts[j]);
               logCP = log(_CalculateCP(bnn, 
                            bnn->eventCounts[j][k], bnn->rowCounts[j]));

               if(data->p0 > bnn->eventCounts[j][k] / bnn->rowCounts[j] && 
                           bnn->eventCounts[j][k] / bnn->rowCounts[j] > 0) {
                  data->p0 = bnn->eventCounts[j][k] / bnn->rowCounts[j];
               }

               data->avgDataLL += prob * logCP;
            }
         }
      }
   }
}

static void _UpdateNetScore(BNUserData netData) {
   if(netData->changedOne) {
      _UpdateNodeAveDataLL(netData->changedOne);
   }
   //if(netData->changedTwo) {
   //   _UpdateNodeAveDataLL(netData->changedTwo);
   //}
}

static double _GetNodeScore(BeliefNetNode bnn) {
   BNNodeUserData data = BNNodeGetUserData(bnn);
   
   if(data->current == 0 || data->isChangedFromCurrent) {
      return data->avgDataLL;
   } else {
      return data->current->avgDataLL;
   }
}

double _GetDeltaScore(BNUserData firstData, BNUserData secondData) {
   double scoreOne, scoreTwo;
   
   scoreOne = scoreTwo = 0;

   if(firstData->changedOne) {
      scoreOne += _GetNodeScore(firstData->changedOne);
      scoreTwo += _GetNodeScore(_BNGetNodeByID(secondData, 
                                BNNodeGetID(firstData->changedOne)));
   } else if(secondData->changedOne) {
      scoreOne += _GetNodeScore(_BNGetNodeByID(firstData, 
                                BNNodeGetID(secondData->changedOne)));
      scoreTwo += _GetNodeScore(secondData->changedOne);
   }

   DebugMessage(1, 4, "    first %lf second %lf\n", scoreOne, scoreTwo);
   return scoreOne - scoreTwo;
}

static int _IsFirstNetBetter(BNUserData firstData, BNUserData secondData) {
   return _GetDeltaScore(firstData, secondData) >= 0;
}

static int _IsCurrentNet(BNUserData data) {
   return (data->changedOne == 0);
             // && (data->changedTwo == 0);
}

static void _PickWinnerInTieFreeRest(ModelSearch ms, int bestIndex) {
   BNUserData winner;
   int i;

   /* use best in tie */
   winner = VLRemove(ms->choices, bestIndex);

   for(i = VLLength(ms->choices) - 1 ; i >= 0 ; i--) {
      //if(_IsCurrentNet(VLIndex(ms->choices, i))) {
      //   /* This favors the current net in ties */
      //   gNumCurrentInTie++;
      //   _FreeUserData(winner);
      //   BNFree(winner);
      //   winner = VLRemove(ms->choices, i);
      //} else {
         _FreeUserData(VLRemove(ms->choices, i));
      //}
   }

   if(_IsCurrentNet(winner)) {
      gNumCurrentInTie++;
   }

   VLAppend(ms->choices, winner);
}

void _GetOneStepChoicesForSearch(ModelSearch ms) {
   int i;
   BeliefNetNode currentNode, parentNode;
   //BNNodeUserData data;
   BNUserData netData;
   long preAllocation;

   ms->initialExampleNumber = -1;

   DebugMessage(VLLength(ms->choices) > 0, 0,
            "******Warning list in choices not empty\n");

   preAllocation = MGetTotalAllocation();

   /* the no-change-net */
   VLAppend(ms->choices,
                 _InitUserData(ms->nodeID, 0, -1, 0, ms->currentModel));

   DebugMessage(1, 3, " allocated current net, size %ld\n",
        MGetTotalAllocation() - preAllocation);

   for(i = 0 ; i < BNGetNumNodes(ms->currentModel) ; i++) {
      if(i != ms->nodeID) {
         currentNode = BNGetNodeByID(ms->currentModel, ms->nodeID);
         parentNode = BNGetNodeByID(ms->currentModel, i);

         /* if they aren't related consider adding dst as a parent */
         if(BNNodeLookupParentIndex(currentNode, parentNode) == -1 &&
               BNNodeLookupParentIndex(parentNode, currentNode) == -1) {

            preAllocation = MGetTotalAllocation();

            netData  = _InitUserData(ms->nodeID, 1, i, 1, ms->currentModel);

            DebugMessage(1, 4, " allocated new net, size %ld\n",
                   MGetTotalAllocation() - preAllocation);

            if((gMaxParentsPerNode == -1 ||
                BNNodeGetNumParents(netData->changedOne) <= 
                                            gMaxParentsPerNode) &&

               (gMaxParameterCount == -1 ||
                 (BNNodeGetNumParameters(netData->changedOne) <=
                                                gMaxParameterCount)) &&

               (ms->linkChangeCounts[i] <= 2) &&
              (gLimitBytes == -1 ||
            ((MGetTotalAllocation() - preAllocation) <=  gMaxBytesPerModel))&&
                   !_BNHasCycle(netData)) {
               VLAppend(ms->choices, netData);
            } else {
               if(gMaxParentsPerNode != -1 &&
                        BNNodeGetNumParents(netData->changedOne) > 
                                                    gMaxParentsPerNode) {
                  gNumByParentLimit++;
               } else if(gLimitBytes != -1 &&
                       ((MGetTotalAllocation() - preAllocation) >
                                                gMaxBytesPerModel)) {
                  gNumByMemoryLimit++;
               } else if(ms->linkChangeCounts[i] > 2) {
                  gNumByChangeLimit++;
               } else if(gMaxParameterCount != -1 && 
                     (BNNodeGetNumParameters(netData->changedOne)
                                              > gMaxParameterCount)) {
                  gNumByParameterLimit++;
               }

               _FreeUserData(netData);
            }
         } else if(gAllowRemove && 
                 BNNodeLookupParentIndex(currentNode, parentNode) != -1) {
            /* Consider removing the link */
            preAllocation = MGetTotalAllocation();

            netData = _InitUserData(ms->nodeID, 1, i, 0, ms->currentModel);

            DebugMessage(1, 3, " allocated new net, size %ld\n",
                   MGetTotalAllocation() - preAllocation);

            if((gMaxParameterCount == -1 ||
                    (BNNodeGetNumParameters(netData->changedOne) <= 
                                                  gMaxParameterCount)) &&

               (ms->linkChangeCounts[i] <= 2) &&

               (gLimitBytes == -1 ||
                      ((MGetTotalAllocation() - preAllocation) 
                                   <=  gMaxBytesPerModel)) &&

                  !_BNHasCycle(netData)) {
               VLAppend(ms->choices, netData);
            } else {
               if(gLimitBytes != -1 &&
                       ((MGetTotalAllocation() - preAllocation) >
                                                gMaxBytesPerModel)) {
                  gNumByMemoryLimit++;
               } else if(ms->linkChangeCounts[i] > 2) {
                  gNumByChangeLimit++;
               } else if(gMaxParameterCount != -1 && 
                     (BNNodeGetNumParameters(netData->changedOne) > 
                                                     gMaxParameterCount)) {
                  gNumByParameterLimit++;
               }

               _FreeUserData(netData);
            }
         }
      }
   }
}



static void _PickCurrentBestAsWinner(ModelSearch ms) {
   int i, bestIndex;
   BNUserData bestNet, compareNet;

   _UpdateNodeAveDataLL(BNGetNodeByID(ms->currentModel, ms->nodeID));

   /* do one pass to update scores & find best */
   bestNet = VLIndex(ms->choices, 0);
   bestIndex = 0;
   _UpdateNetScore(bestNet);

   if(VLLength(ms->choices) < 2) {
      /* we must have found a winner somehow, maybe only ever made 1*/
      DebugMessage(1, 3, "*******Warning: comparing one pick best\n");

      if(_IsCurrentNet(VLIndex(ms->choices, 0))) {
         gNumCurrentByWin++;
      }
      return;
   }

   for(i = 1 ; i < VLLength(ms->choices) ; i++) {
      compareNet = VLIndex(ms->choices, i);
      _UpdateNetScore(compareNet);


      if(!_IsFirstNetBetter(bestNet, compareNet)) {
         bestNet = compareNet;
         bestIndex = i;
      }

      //if(!_IsFirstNetBetter(bestNet, compareNet) || 
      //                            _IsCurrentNet(compareNet)) {
      //   if(!_IsCurrentNet(bestNet)) {
      //      bestNet = compareNet;
      //      bestIndex = i;
      //   }
      //}
   }

   for(i = VLLength(ms->choices) - 1 ; i >= 0 ; i--) {
      if(i != bestIndex) {
         _FreeUserData(VLRemove(ms->choices, i));
      }
   }

   if(_IsCurrentNet(VLIndex(ms->choices, 0))) {
      gNumCurrentInTie++;
   }
}

static void _CompareNetsBoundFreeLoosers(ModelSearch ms) {
//BeliefNet current, VoidListPtr netChoices) {

   int i, bestIndex, secondIndex;
   BNUserData bestNet, secondNet, compareNet;
   double deltaScore;
   double epsilon;
   double scoreRangeNode;

   _UpdateNodeAveDataLL(BNGetNodeByID(ms->currentModel, ms->nodeID));

   /* do one pass to update scores & find two best nets */
   bestNet = VLIndex(ms->choices, 0);
   bestIndex = 0;
   _UpdateNetScore(bestNet);

   if(VLLength(ms->choices) < 2) {
      /* we must have found a winner somehow, maybe only ever made 1*/
      DebugMessage(1, 3, "*******Warning: comparing one\n");

      if(_IsCurrentNet(VLIndex(ms->choices, 0))) {
         gNumCurrentByWin++;
      }

      return;
   }

   compareNet = VLIndex(ms->choices, 1);
   _UpdateNetScore(compareNet);
   if(!_IsFirstNetBetter(bestNet, compareNet)) {
      secondNet = bestNet;
      secondIndex = bestIndex;

      bestNet = compareNet;
      bestIndex = 1;
   } else {
      secondNet = compareNet;
      secondIndex = 1;
   }


   for(i = 2 ; i < VLLength(ms->choices) ; i++) {
      compareNet = VLIndex(ms->choices, i);
      _UpdateNetScore(compareNet);

//printf("id %d, changedParent %d, complexity %d\n", ms->nodeID, bestNet->changedParentID, bestNet->changeComplexity);
//printf("id %d, changedParent %d, complexity %d\n", ms->nodeID, compareNet->changedParentID, compareNet->changeComplexity);
      if(!_IsFirstNetBetter(bestNet, compareNet)) {
         secondNet = bestNet;
         secondIndex = bestIndex;

         bestNet = compareNet;
         bestIndex = i;
      } else if(!_IsFirstNetBetter(secondNet, compareNet)) {
         secondNet = compareNet;
         secondIndex = i;
      }
   }

   /* see if the bestNet beats the secondNet */
   deltaScore = _GetDeltaScore(bestNet, secondNet);

   scoreRangeNode = fabs(_GetComparedNodesScoreRange(bestNet, secondNet));

   /* HERE for missing data or unequal in compare */
   epsilon = _GetEpsilonNormal(bestNet, secondNet, ms->currentModel);

   DebugMessage(1, 3, " delta %lf epsilon %lf nodeRange: %f times-tau: %f\n",
          deltaScore, epsilon, scoreRangeNode, scoreRangeNode * gTau);
   if(deltaScore - epsilon > 0) {
      DebugMessage(1, 2,
               "Finish by winner, diff %f, epsilon %f were %d left\n", 
               deltaScore, epsilon, VLLength(ms->choices));

      gNumByWin += VLLength(ms->choices) - 1;
      gNumBoundsUsed += VLLength(ms->choices) - 1;

      if(_IsCurrentNet(bestNet)) {
         gNumCurrentByWin++;
      }

      for(i = VLLength(ms->choices) - 1 ; i >= 0 ; i--) {
         if(i != bestIndex) {
            _FreeUserData(VLRemove(ms->choices, i));
         }
      }
   } else {
      /* tie if it won't cost more than range * tau */
      if(fabs(deltaScore) <= epsilon &&
                    epsilon <= (gTau * scoreRangeNode)) {
         /* that's a tie */

         DebugMessage(1, 2, 
                "Finish by tie diff %f epsilon %f range %f with %d left\n",
             deltaScore, epsilon, scoreRangeNode, VLLength(ms->choices));
         //DebugMessage(1, 3, "  best %p compare %p\n", bestNet, secondNet);

         gNumByTie += VLLength(ms->choices) - 1;
         gNumBoundsUsed += VLLength(ms->choices) - 1;

         _PickWinnerInTieFreeRest(ms, bestIndex);

      } else if(fabs(deltaScore) < epsilon) {
         /* see if we could possibly call a tie */
         //if(tauNeededToQuitNode < epsilon / scoreRangeNode) {
         //   tauNeededToQuitNode = epsilon / scoreRangeNode;
         //}
      }
   }

   /* if not do another pass deactivating clear loosers */
   if(VLLength(ms->choices) > 1) {
      for(i = VLLength(ms->choices) - 1 ; i >= 0 ; i--) {
         if(i != bestIndex && i != secondIndex) {
            compareNet = VLIndex(ms->choices, i);
            deltaScore = _GetDeltaScore(secondNet, compareNet);
            scoreRangeNode = 
                  fabs(_GetComparedNodesScoreRange(secondNet, compareNet));

            /* HERE for missing data or unequal in compare */
            epsilon = _GetEpsilonNormal(secondNet, compareNet,
                                                ms->currentModel);

            if(deltaScore - epsilon > 0) {
               DebugMessage(1, 2, 
                 "Early freeing a net, diff %f, epsilon %f now %d left\n", 
                  deltaScore, epsilon, VLLength(ms->choices));
               gNumByWin++;
               gNumBoundsUsed++;

               if(_IsCurrentNet(compareNet)) {
                  DebugMessage(1, 2, 
                    " Current best freed bound - delta %lf epsilon %lf.\n", 
                                                    deltaScore, epsilon);
               }
               _FreeUserData(VLRemove(ms->choices, i));
            } 
         }
      }
   }
}

ModelSearch MSNew(BeliefNet currentBN, int nodeID) {
   int i;
   ModelSearch ms = MNewPtr(sizeof(ModelSearchStruct));

   ms->initialExampleNumber = -1;
   ms->lastParentAdded = -1;
   ms->isActive = 0;

   ms->choices = VLNew();
   ms->isDone = 0;
   ms->doneCollectingExamples = 0;

   ms->currentModel = currentBN;
   ms->nodeID       = nodeID;

   /* MEM this may leak some memory, but should only happen once */
   _InitCurrentUserData(currentBN);

   ms->linkChangeCounts = MNewPtr(sizeof(int) * BNGetNumNodes(currentBN));
   for(i = 0 ; i < BNGetNumNodes(currentBN) ; i++) {
      ms->linkChangeCounts[i] = 0;
   }

   return ms;
}

void MSFree(ModelSearch ms) {
   MSDeactivate(ms);
   VLFree(ms->choices);
   MFreePtr(ms->linkChangeCounts);
   MFreePtr(ms);
}

int MSGetNodeID(ModelSearch ms) {
   return ms->nodeID;
}

void MSAddExample(ModelSearch ms, ExamplePtr e, long exampleNumber) {
   int i;
   BNUserData netData;

   if(ms->initialExampleNumber == -1) {
      ms->initialExampleNumber = exampleNumber;
   } else if(exampleNumber == ms->initialExampleNumber) {
      /* we are at the end of looping through all the examples */
      /*  so just pick the best one as the winner and maybe print warning */

      ms->doneCollectingExamples = 1;

      gNumUsedWholeDB++;

      DebugMessage(1, 0, "******Looped through all examples\n");
   }

   if(!ms->doneCollectingExamples) {
      BNNodeAddSample(BNGetNodeByID(ms->currentModel, ms->nodeID), e);
      for(i = 0 ; i < VLLength(ms->choices) ; i++) {
         netData = VLIndex(ms->choices, i);
         if(netData->changedOne) {
            BNNodeAddSample(netData->changedOne, e);
         }
      }
   }
}

void MSTerminateSearch(ModelSearch ms) {
   ms->doneCollectingExamples = 1;
}

int MSIsDone(ModelSearch ms) {
   return ms->isDone;
}

static void _ApplyWinner(ModelSearch ms) {
   BNUserData winnerData;
   BNNodeUserData nodeData;
   BeliefNetNode changedNode;
   long samples;

   winnerData = VLRemove(ms->choices, 0);

   samples = BNNodeGetNumSamples(_BNGetNodeByID(winnerData, ms->nodeID));

   if(samples > 0) {
      gMinSamplesInDecision = min(samples, gMinSamplesInDecision);
      gMaxSamplesInDecision = max(samples, gMaxSamplesInDecision);
      gTotalSamplesInDecision += samples;
   } else {
      gNumZeroSamplesInDecision++;
   }


   /* if doing nothing wins then set isDone */
   if(_IsCurrentNet(winnerData)) {
      DebugMessage(1, 2, "Current net wins for %d\n", ms->nodeID);
      ms->isDone = 1;
   } else if(!_BNHasCycle(winnerData)) {
      ms->linkChangeCounts[winnerData->changedParentID]++;

      /* apply any winner to the current model */
      BNFlushStructureCache(ms->currentModel);
      BNNodeFreeCPT(BNGetNodeByID(ms->currentModel, ms->nodeID));

      if(winnerData->changeComplexity == -1) {
         gNumRemoved++;

         changedNode = BNGetNodeByID(ms->currentModel, ms->nodeID);
         BNNodeRemoveParent(changedNode,
           BNNodeLookupParentIndexByID(changedNode,
                                 winnerData->changedParentID));

         nodeData = BNNodeGetUserData(changedNode);
         nodeData->p0 = 1.0;

         DebugMessage(1, 2, "Winner for %d remove parent %d now %d parents\n", 
            ms->nodeID, winnerData->changedParentID,
             BNNodeGetNumParents(BNGetNodeByID(ms->currentModel, ms->nodeID)));
      } else if(winnerData->changeComplexity == 2) {
         gNumAdded++;

         changedNode = BNGetNodeByID(ms->currentModel, ms->nodeID);
         BNNodeAddParent(changedNode,
              BNGetNodeByID(ms->currentModel, winnerData->changedParentID));

         nodeData = BNNodeGetUserData(changedNode);
         nodeData->p0 = 1.0;

         DebugMessage(1, 2, "Winner for %d new parent %d now %d parents\n", 
            ms->nodeID, winnerData->changedParentID,
             BNNodeGetNumParents(BNGetNodeByID(ms->currentModel, ms->nodeID)));
      }


      BNNodeInitCPT(BNGetNodeByID(ms->currentModel, ms->nodeID));

      /* EFF I don't think I need this SetCPTFrom call */
      //BNNodeSetCPTFrom(BNGetNodeByID(ms->currentModel, ms->nodeID),
      //              BNGetNodeByID(winnerData->changedOne, ms->nodeID));

   }

   _FreeUserData(winnerData);

   MSDeactivate(ms);
}

int MSCheckForWinner(ModelSearch ms) {
   int foundWinner;

   if(MSIsDone(ms) || !MSIsActive(ms)) {
      foundWinner = 0;
   } else if(ms->doneCollectingExamples) {
      _PickCurrentBestAsWinner(ms);
      foundWinner = 1;
   } else {
      _CompareNetsBoundFreeLoosers(ms);

      if(VLLength(ms->choices) == 1) {
         foundWinner = 1;
      } else {
         foundWinner = 0;
      }
   }

   if(foundWinner) {
      _ApplyWinner(ms);
   }

   return foundWinner;
}

void MSModelChangedHandleConflicts(ModelSearch ms) {
   int i;
   BNUserData netData;

   /* apply the changes to each of the search alternatives */
   /* check each for cycles and if there are any free them */
   for(i = VLLength(ms->choices) - 1 ; i >= 0 ; i--) {
      netData = VLIndex(ms->choices, i);
      if(_BNHasCycle(netData)) {
         gNumByCycleConflict++;
         _FreeUserData(netData);
         VLRemove(ms->choices, i);

         //} else if(gMaxParameterCount != -1 &&
         //              (BNGetNumParameters(bn) > gMaxParameterCount)) {
         //      gNumByParameterConflict++;
         //      _FreeUserData(netData);
         //      VLRemove(ms->choices, i);
      }
   }

   if(VLLength(ms->choices) == 1) {
      /* if only one left then handle the winner */
      if(_IsCurrentNet(VLIndex(ms->choices, 0))) {
         gNumCurrentByDefault++;
      }
      _ApplyWinner(ms);
   } else if(VLLength(ms->choices) == 0) {
      /* if everyone is free then deactivate and let someone else cope */      
      MSDeactivate(ms);
   }
}


int MSIsActive(ModelSearch ms) {
   return ms->isActive;
}

void MSActivate(ModelSearch ms) {
   if(!ms->isActive) {
      _GetOneStepChoicesForSearch(ms);
      ms->isActive = 1;
      ms->doneCollectingExamples = 0;

      ms->numSearchOptions = VLLength(ms->choices);

      DebugMessage(1, 3, "    activated with %d alternatives\n",
                              ms->numSearchOptions);

      if(ms->numSearchOptions <= 1) {
         gNumCurrentByDefault++;
      }
   }
}

void MSDeactivate(ModelSearch ms) {
   int i;

   if(ms->isActive) {
      for(i = VLLength(ms->choices) - 1 ; i >= 0 ; i--) {
         _FreeUserData(VLRemove(ms->choices, i));
      }

      ms->isActive = 0;
   }
}

int MSNumActiveAlternatives(ModelSearch ms) {
   return VLLength(ms->choices);
}
