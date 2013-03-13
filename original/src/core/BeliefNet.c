#include "vfml.h"

#include <math.h>
#include <assert.h>


static BeliefNetNode _BNNodeNew(BeliefNet bn) {
   BeliefNetNode bnn = (BeliefNetNode)MNewPtr(sizeof(BeliefNetNodeStruct));

   bnn->numAllocatedRows = 0;

   bnn->numParentCombinations = -1;

   bnn->eventCounts = 0;
   bnn->rowCounts = 0;

   bnn->numSamples = 0;

   bnn->bn = bn;

   bnn->spec = 0;
   bnn->attributeID = -1;

   bnn->parentIDs = VLNew();
   bnn->childrenIDs = VLNew();

   bnn->userData = 0;

   return bnn;
}

void BNNodeFree(BeliefNetNode bnn) {
   BNNodeFreeCPT(bnn);

   VLFree(bnn->parentIDs);
   VLFree(bnn->childrenIDs);

   MFreePtr(bnn);
}

static int _GetNumParentCombinations(BeliefNetNode bnn) {
   int numParentCombinations;
   BeliefNetNode parent;
   int i;

   if(bnn->numParentCombinations == -1) {
      numParentCombinations = 1;
      for(i = 0 ; i < BNNodeGetNumParents(bnn) ; i++) {
         parent = BNNodeGetParent(bnn, i);
         numParentCombinations *= BNNodeGetNumValues(parent);
      }
      bnn->numParentCombinations = numParentCombinations;
   }
   return bnn->numParentCombinations;
}

void BNNodeSetCPTFrom(BeliefNetNode target, BeliefNetNode source) {
   int i, j, numParentCombinations, numValues;

   /* NOTE! Both nodes better have CPTs allocated and they better be
         the same */
   target->numSamples = source->numSamples;

   numValues = BNNodeGetNumValues(target);
   numParentCombinations = _GetNumParentCombinations(target);

   for(i = 0 ; i < numParentCombinations ; i++) {
      target->rowCounts[i] = source->rowCounts[i];

      for(j = 0 ; j < numValues ; j++) {
         target->eventCounts[i][j] = source->eventCounts[i][j];
      }
   }
}

/* clone used to make a new BN, so make sure all dependencies in the
    clone point to values in the new net.  */
BeliefNetNode _BNNodeClone(BeliefNetNode bnn, BeliefNet newBN) {
   int i,j, numParentCombinations, numValues;
   BeliefNetNode newBnn = _BNNodeNew(newBN);

   newBnn->spec = newBN->spec;
   newBnn->attributeID = bnn->attributeID;

   for(i = 0 ; i < VLLength(bnn->parentIDs) ; i++) {
      VLAppend(newBnn->parentIDs, VLIndex(bnn->parentIDs, i));
   }

   for(i = 0 ; i < VLLength(bnn->childrenIDs) ; i++) {
      VLAppend(newBnn->childrenIDs, VLIndex(bnn->childrenIDs, i));
   }

   newBnn->userData = bnn->userData;
   newBnn->numParentCombinations = bnn->numParentCombinations;

   if(bnn->eventCounts != 0) {
      // Then copy the CPT
      numValues = BNNodeGetNumValues(newBnn);

      numParentCombinations = _GetNumParentCombinations(newBnn);

      newBnn->numAllocatedRows = numParentCombinations;
      newBnn->rowCounts = 
               (float *)MNewPtr(sizeof(float) * numParentCombinations);
      newBnn->eventCounts = 
               (float **)MNewPtr(sizeof(float *) * numParentCombinations);

      for(i = 0 ; i < numParentCombinations ; i++) {
         newBnn->rowCounts[i] = bnn->rowCounts[i];

         newBnn->eventCounts[i] = (float *)MNewPtr(sizeof(float) * numValues);
         for(j = 0 ; j < numValues ; j++) {
            newBnn->eventCounts[i][j] = bnn->eventCounts[i][j];
         }
      }
   }

   newBnn->numSamples = bnn->numSamples;

   return newBnn;
}


/* clone used to make a new BN, so make sure all dependencies in the
    clone point to values in the new net. */
BeliefNetNode _BNNodeCloneNoCPT(BeliefNetNode bnn, BeliefNet newBN) {
   int i;
   BeliefNetNode newBnn = _BNNodeNew(newBN);


   newBnn->spec = newBN->spec;
   newBnn->attributeID = bnn->attributeID;

   for(i = 0 ; i < VLLength(bnn->parentIDs) ; i++) {
      VLAppend(newBnn->parentIDs, VLIndex(bnn->parentIDs, i));
   }

   for(i = 0 ; i < VLLength(bnn->childrenIDs) ; i++) {
      VLAppend(newBnn->childrenIDs, VLIndex(bnn->childrenIDs, i));
   }

   newBnn->userData = bnn->userData;
   newBnn->numParentCombinations = bnn->numParentCombinations;

   newBnn->eventCounts = 0;
   newBnn->numSamples = 0;

   return newBnn;
}


void BNNodeAddParent(BeliefNetNode bnn, BeliefNetNode parent) {
   VLAppend(bnn->parentIDs, (void *)parent->attributeID);
   VLAppend(parent->childrenIDs, (void *)bnn->attributeID);

   bnn->numParentCombinations = -1;
}

int BNNodeLookupParentIndex(BeliefNetNode bnn, BeliefNetNode parent) {
   return BNNodeLookupParentIndexByID(bnn, BNNodeGetID(parent));
}

int BNNodeLookupParentIndexByID(BeliefNetNode bnn, int id) {
   int i;

   for(i = 0 ; i < VLLength(bnn->parentIDs) ; i++) {
      if(BNNodeGetParentID(bnn, i) == id) {
         return i;
      }
   }

   return -1;
}

void BNNodeRemoveParent(BeliefNetNode bnn, int parentIndex) {
   int i, done;
   BeliefNetNode parent;

   bnn->numParentCombinations = -1;

   parent = BNNodeGetParent(bnn, parentIndex);

   VLRemove(bnn->parentIDs, parentIndex);

   done = 0;
   for(i = 0 ; i < VLLength(parent->childrenIDs) && !done ; i++) {
      if((int)VLIndex(parent->childrenIDs, i) == BNNodeGetID(bnn)) {
         VLRemove(parent->childrenIDs, i);
         done = 1;
      }
   }
}

BeliefNetNode BNNodeGetParent(BeliefNetNode bnn, int parentIndex) {
   return BNGetNodeByID(bnn->bn, (int)VLIndex(bnn->parentIDs, parentIndex));
}

int BNNodeGetParentID(BeliefNetNode bnn, int parentIndex) {
   return (int)VLIndex(bnn->parentIDs, parentIndex);
}

int BNNodeGetNumParents(BeliefNetNode bnn) {
   return VLLength(bnn->parentIDs);
}

int BNNodeGetNumChildren(BeliefNetNode bnn) {
   return VLLength(bnn->childrenIDs);
}

int BNNodeHasParent(BeliefNetNode bnn, BeliefNetNode parent) {
   int i;

   for(i = 0 ; i < VLLength(bnn->parentIDs) ; i++) {
      if(BNNodeGetParentID(bnn, i) == BNNodeGetID(parent)) {
         return 1;
      }
   }

   return 0;
}

int BNNodeHasParentID(BeliefNetNode bnn, int parentID) {
   int i;

   for(i = 0 ; i < VLLength(bnn->parentIDs) ; i++) {
      if(BNNodeGetParentID(bnn, i) == parentID) {
         return 1;
      }
   }

   return 0;
}

void BNNodeAddValue(BeliefNetNode bnn, char *valueName) {
   ExampleSpecAddAttributeValue(bnn->spec, BNNodeGetID(bnn), valueName);
}

int BNNodeLookupValue(BeliefNetNode bnn, char *valueName) {
   return ExampleSpecLookupAttributeValueName(bnn->spec, BNNodeGetID(bnn),
                         valueName);
}

int BNNodeGetNumValues(BeliefNetNode bnn) {
   return ExampleSpecGetAttributeValueCount(bnn->spec, BNNodeGetID(bnn));
}

int BNNodeGetNumParameters(BeliefNetNode bnn) {
   return BNNodeGetNumCPTRows(bnn) * BNNodeGetNumValues(bnn);
}

char *BNNodeGetName(BeliefNetNode bnn) {
   return ExampleSpecGetAttributeName(bnn->spec, BNNodeGetID(bnn));
}

int BNNodeStructureEqual(BeliefNetNode bnn, BeliefNetNode otherNode) {
   int i;

   if(BNNodeGetNumParents(bnn) != BNNodeGetNumParents(otherNode)) {
      return 0;
   } else {
      for(i = 0 ; i < BNNodeGetNumParents(bnn) ; i++) {
         if(BNNodeGetParentID(bnn, i) != BNNodeGetParentID(otherNode, i)) {
            return 0;
         }
      }
   }

   return 1;
}

static void _BNNodeSetIdentity(BeliefNetNode bnn, ExampleSpecPtr spec, 
                                                               int id) {
   bnn->spec = spec;
   bnn->attributeID = id;
}

void BNNodeInitCPT(BeliefNetNode bnn) {
   int numValues;
   int numParentCombinations;
   int i,j;

   DebugError(bnn->attributeID == -1, "Initing CPT before identifying node");
   numValues = BNNodeGetNumValues(bnn);

   numParentCombinations = _GetNumParentCombinations(bnn);
   //printf("num parent combs %d\n", numParentCombinations);

   bnn->numAllocatedRows = numParentCombinations;
   bnn->rowCounts = (float *)MNewPtr(sizeof(float) * numParentCombinations);
   bnn->eventCounts = 
                (float **)MNewPtr(sizeof(float *) * numParentCombinations);

   for(i = 0 ; i < numParentCombinations ; i++) {
      bnn->rowCounts[i] = 0;

      bnn->eventCounts[i] = (float *)MNewPtr(sizeof(float) * numValues);
      for(j = 0 ; j < numValues ; j++) {
         bnn->eventCounts[i][j] = 0;
      }
   }

   bnn->numSamples = 0;
}

void BNNodeZeroCPT(BeliefNetNode bnn) {
   int numValues;
   int i,j;

   DebugError(bnn->eventCounts == 0, "Zeroing CPT before initing");
   numValues = BNNodeGetNumValues(bnn);

   for(i = 0 ; i < bnn->numAllocatedRows ; i++) {
      bnn->rowCounts[i] = 0;
      for(j = 0 ; j < numValues ; j++) {
         bnn->eventCounts[i][j] = 0;
      }
   }

   bnn->numSamples = 0;
}

void BNNodeFreeCPT(BeliefNetNode bnn) {
   int i;

   if(bnn->eventCounts != 0) {
      for(i = 0 ; i < bnn->numAllocatedRows ; i++) {
         MFreePtr(bnn->eventCounts[i]);
      }

      MFreePtr(bnn->eventCounts);
      bnn->eventCounts = 0;
   }

   if(bnn->rowCounts != 0) {
      MFreePtr(bnn->rowCounts);
      bnn->rowCounts = 0;
   }

   bnn->numAllocatedRows = 0;
   bnn->numSamples = 0;
}

int BNNodeGetID(BeliefNetNode bnn) {
   return bnn->attributeID;
}

void BNNodeSetUserData(BeliefNetNode bnn, void *data) {
   bnn->userData = data;
}

//void *BNNodeGetUserData(BeliefNetNode bnn) {
//   return bnn->userData;
//}

static int _FindEventRowIndex(BeliefNetNode bnn, ExamplePtr e) {
   int i, answer, offset, done;
   int parentID;

   //printf("finding index for: ");
   //ExampleWrite(e, stdout);

   answer = 0;
   done = 0;
   offset = 1;
   for(i = VLLength(bnn->parentIDs) - 1 ; i >= 0 && !done ; i--) {
      parentID = (int)VLIndex(bnn->parentIDs, i);

      if(ExampleIsAttributeUnknown(e, parentID)) {
         answer = -1;
         done = 1;
      } else {
         answer += (ExampleGetDiscreteAttributeValueUnsafe(e, parentID) * offset);
         offset *= ExampleSpecGetAttributeValueCount(bnn->spec, parentID);
      }
   }

   //printf("answer is %d\n", answer);

   return answer;
}

//static float *_GetEventRow(BeliefNetNode bnn, ExamplePtr e) {
//   int rowIndex = _FindEventRowIndex(bnn, e);
//
//   if(rowIndex == -1) {
//      return 0;
//   } else {
//      return bnn->eventCounts[rowIndex];
//   }
//}

void  BNNodeAddSample(BeliefNetNode bnn, ExamplePtr e) {
   BNNodeAddFractionalSample(bnn, e, 1.0);
}
void BNNodeAddSamples(BeliefNetNode bnn, VoidListPtr samples) {
   BNNodeAddFractionalSamples(bnn, samples, 1.0);
}

void  BNNodeAddFractionalSample(BeliefNetNode bnn, ExamplePtr e, float weight){
   int rowIndex = _FindEventRowIndex(bnn, e);
   float *eventRow;

   if(rowIndex != -1) {
      eventRow = bnn->eventCounts[rowIndex];
      if(!ExampleIsAttributeUnknown(e, BNNodeGetID(bnn))) {
         bnn->rowCounts[rowIndex] += weight;
         eventRow[ExampleGetDiscreteAttributeValueUnsafe(e, BNNodeGetID(bnn))] +=
                                                              weight;
         bnn->numSamples += weight;
      }
   }
}

/*
void  BNNodeAddFractionalSamples(BeliefNetNode bnn, VoidListPtr samples, float weight){
   int i;
   int id=BNNodeGetID(bnn);
   int numSamples = VLLength(samples);
   for (i=0; i<numSamples; i++) {
      ExamplePtr e = VLIndex(samples, i);
      int rowIndex = _FindEventRowIndex(bnn, e);
      if(rowIndex != -1) {
         float *eventRow = bnn->eventCounts[rowIndex];
         if(!ExampleIsAttributeUnknown(e, id)) {
            bnn->rowCounts[rowIndex] += weight;
            eventRow[ExampleGetDiscreteAttributeValueUnsafe(e, id)] += weight;
         }
         bnn->numSamples += weight;
      }
   }
}
*/

// Assumes all are known
void  BNNodeAddFractionalSamples(BeliefNetNode bnn, VoidListPtr samples, float weight){
   int i;
   ExamplePtr e;
   int rowIndex;   
   int id=BNNodeGetID(bnn);
   int numSamples = VLLength(samples);
   int numSamples_rounded = 4*(numSamples/4);
   
   for (i=0; i<numSamples_rounded;) {
      e = VLIndex(samples, i); rowIndex = _FindEventRowIndex(bnn, e);
      bnn->rowCounts[rowIndex] += weight;
      bnn->eventCounts[rowIndex][ExampleGetDiscreteAttributeValueUnsafe(e, id)] += weight;
      i++;

      e = VLIndex(samples, i); rowIndex = _FindEventRowIndex(bnn, e);
      bnn->rowCounts[rowIndex] += weight;
      bnn->eventCounts[rowIndex][ExampleGetDiscreteAttributeValueUnsafe(e, id)] += weight;
      i++;

      e = VLIndex(samples, i); rowIndex = _FindEventRowIndex(bnn, e);
      bnn->rowCounts[rowIndex] += weight;
      bnn->eventCounts[rowIndex][ExampleGetDiscreteAttributeValueUnsafe(e, id)] += weight;
      i++;

      e = VLIndex(samples, i); rowIndex = _FindEventRowIndex(bnn, e);
      bnn->rowCounts[rowIndex] += weight;
      bnn->eventCounts[rowIndex][ExampleGetDiscreteAttributeValueUnsafe(e, id)] += weight;
      i++;
   }
   for (; i<numSamples; ) {
      e = VLIndex(samples, i); rowIndex = _FindEventRowIndex(bnn, e);
      bnn->rowCounts[rowIndex] += weight;
      bnn->eventCounts[rowIndex][ExampleGetDiscreteAttributeValueUnsafe(e, id)] += weight;
      i++;
   }
}







float BNNodeGetCPTRowCount(BeliefNetNode bnn, ExamplePtr e) {
   int rowIndex = _FindEventRowIndex(bnn, e);

   if(rowIndex != -1) {
      return bnn->rowCounts[rowIndex];
   } else {
      return 0;
   }
}


float BNNodeGetP(BeliefNetNode bnn, int value) {
   int row, i;
   int eventTotal=0, valueEventTotal=0;
   int numRows = _GetNumParentCombinations(bnn);
   int numVals = BNNodeGetNumValues(bnn);   

   for(row = 0 ; row < numRows ; row++) {
      // eventTotal += bnn->rowCounts[row];
      for (i = 0; i < numVals; i++)
         eventTotal += bnn->eventCounts[row][i];
      valueEventTotal += bnn->eventCounts[row][value];
   }
   if (eventTotal == 0) {
      return 0.0;
   }
   else {
      return (double)valueEventTotal / eventTotal;
   }
}

float BNNodeGetCPIndexed(BeliefNetNode bnn, int row, int value) {
   if(bnn->rowCounts[row] == 0) {
      /* HERE is this the right thing? */
      return 0;
   }

   return bnn->eventCounts[row][value] / bnn->rowCounts[row];
}

void  BNNodeSetCPIndexed(BeliefNetNode bnn, int row, int value,
                                                      float probability) {
   if(bnn->rowCounts[row] == 0) {
      bnn->rowCounts[row] = 1;
   }

   if(bnn->numSamples == 0) {
      bnn->numSamples = 1;
   }

   bnn->eventCounts[row][value] = probability * bnn->rowCounts[row];
}

float BNNodeGetCP(BeliefNetNode bnn, ExamplePtr e) {
   int rowIndex = _FindEventRowIndex(bnn, e);
   float *eventRow;
   float event, rowSum;

   //printf("rowIndex %d\n", rowIndex);
   if(rowIndex != -1) {
      //assert(rowIndex >= 0);
      eventRow = bnn->eventCounts[rowIndex];
      if(!ExampleIsAttributeUnknown(e, BNNodeGetID(bnn))) {
         rowSum = bnn->rowCounts[rowIndex];
         event = 
            eventRow[ExampleGetDiscreteAttributeValueUnsafe(e, BNNodeGetID(bnn))];

         //printf("event %f rowSum %f\n", event, rowSum);

         if(rowSum == 0) {
            /* HERE is this the right thing? */
            return 0;
         }

         return event / rowSum;
      }
   }

   return -1;
}


void  BNNodeSetCP(BeliefNetNode bnn, ExamplePtr e, float probability) {
   int rowIndex = _FindEventRowIndex(bnn, e);
   float *eventRow;
   float rowSum;

   if(rowIndex != -1) {
      eventRow = bnn->eventCounts[rowIndex];
      if(!ExampleIsAttributeUnknown(e, BNNodeGetID(bnn))) {
         if(bnn->rowCounts[rowIndex] == 0) {
            bnn->rowCounts[rowIndex] = 1;
         }

         if(bnn->numSamples == 0) {
            bnn->numSamples = 1;
         }

         rowSum = bnn->rowCounts[rowIndex];
         eventRow[ExampleGetDiscreteAttributeValueUnsafe(e, BNNodeGetID(bnn))] =
                      probability * rowSum;
      }
   }
}

void BNNodeSetPriorStrength(BeliefNetNode bnn, double strength) {
   int i, j;
   int numRows = _GetNumParentCombinations(bnn);
   double scale;

   for(i = 0 ; i < numRows ; i++) {
      scale = strength / bnn->rowCounts[i];
      bnn->rowCounts[i] = strength;
      for(j = 0 ; j < BNNodeGetNumValues(bnn) ; j++) {
         bnn->eventCounts[i][j] *= scale;
      }
   }
}

void BNNodeSmoothProbabilities(BeliefNetNode bnn, double strength) {
   int i, j;
   int numRows = _GetNumParentCombinations(bnn);

   for(i = 0 ; i < numRows ; i++) {
      for(j = 0 ; j < BNNodeGetNumValues(bnn) ; j++) {
         bnn->rowCounts[i] += strength;
         bnn->eventCounts[i][j] += strength;
      }
   }
}


void BNNodeSetRandomValueGivenParents(BeliefNetNode bnn, ExamplePtr e) {
   int rowIndex = _FindEventRowIndex(bnn, e);
   float *eventRow;
   int numValues;
   float probabilitySum;
   float randomValue;
   int i, done;

   if(rowIndex != -1) {
      eventRow = bnn->eventCounts[rowIndex];
      numValues = BNNodeGetNumValues(bnn);

      probabilitySum = 0;
      for(i = 0 ; i < numValues ; i++) {
         probabilitySum += eventRow[i] / bnn->rowCounts[rowIndex];
      }

      do {
         randomValue = RandomDouble();
      } while(randomValue == 0);

      randomValue *= probabilitySum;

      done = 0;
      for(i = 0 ; i < numValues && !done; i++) {
         randomValue -= eventRow[i] / bnn->rowCounts[rowIndex];
         if(randomValue <= 0) {
            ExampleSetDiscreteAttributeValue(e, BNNodeGetID(bnn), i);
            done = 1;
         }
      }

      if(!done) {
         // Occasionally the math is off just barely, set to the last
             // value with non-zero probability
         i = numValues - 1;
         while(eventRow[i] == 0 && i > 0) {
            i--;
         }

         ExampleSetDiscreteAttributeValue(e, BNNodeGetID(bnn), i);
      }
   } else {
      DebugWarn(1, "Trying to set value for variable without all parents.");
   }
}

float BNNodeGetNumSamples(BeliefNetNode bnn) {
   return bnn->numSamples;
}

int BNNodeGetNumCPTRows(BeliefNetNode bnn) {
   return _GetNumParentCombinations(bnn);
}


/////////////////////// Belief Net ///////////////////

BeliefNet BNNew(void) {
   BeliefNet bn = (BeliefNet)MNewPtr(sizeof(BeliefNetStruct));
   char *tmpStr;

   bn->name = 0;
   bn->spec = ExampleSpecNew();

   tmpStr = MNewPtr(sizeof(char) * (strlen("None") + 1));
   strcpy(tmpStr, "None");
   ExampleSpecAddClass(bn->spec, tmpStr);

   bn->nodes = VLNew();

   bn->topoSort = 0;
   bn->hasCycle = -1;

   bn->userData = 0;

   return bn;
}

void BNFree(BeliefNet bn) {
   int i;

   /* HERE I can't free this unless I clone it in BNClone */
   //ExampleSpecFree(bn->spec);

   if(bn->name != 0) {
      MFreePtr(bn->name);
   }

   for(i = VLLength(bn->nodes) - 1 ; i >= 0 ; i--) {
      BNNodeFree(VLRemove(bn->nodes, i));
   }
   VLFree(bn->nodes);

   if(bn->topoSort != 0) {
      VLFree(bn->topoSort);
   }

   MFreePtr(bn);
}

BeliefNet BNClone(BeliefNet bn) {
   int i;
   BeliefNet newBN = BNNew();
   char *clonedName;
   BeliefNetNode oldBnn, bnn;

   if(bn->name != 0) {
      clonedName = MNewPtr(sizeof(char) * (strlen(bn->name) + 1));
      strcpy(clonedName, bn->name);
      newBN->name = clonedName;
   }

   ExampleSpecFree(newBN->spec);
   /* HERE maybe I should clone the example spec? */
   newBN->spec = bn->spec;

   for(i = 0 ; i < BNGetNumNodes(bn) ; i++) {
      /* put placeholder nodes in the new network */
      VLAppend(newBN->nodes, 0);
   }

   for(i = 0 ; i < BNGetNumNodes(bn) ; i++) {
      bnn = BNGetNodeByElimOrder(bn, i);
      VLSet(newBN->nodes, BNNodeGetID(bnn), _BNNodeClone(bnn, newBN));
   }

   if(bn->topoSort != 0) {
      newBN->topoSort = VLNew();
      for(i = 0 ; i < VLLength(bn->topoSort) ; i++) {
         oldBnn = VLIndex(bn->topoSort, i);
         VLAppend(newBN->topoSort, VLIndex(newBN->nodes, BNNodeGetID(oldBnn)));
      }
   }

   newBN->hasCycle = bn->hasCycle;

   return newBN;
}


BeliefNet BNCloneNoCPTs(BeliefNet bn) {
   int i;
   BeliefNet newBN = BNNew();
   char *clonedName;
   BeliefNetNode oldBnn, bnn;

   if(bn->name != 0) {
      clonedName = MNewPtr(sizeof(char) * (strlen(bn->name) + 1));
      strcpy(clonedName, bn->name);
      newBN->name = clonedName;
   }

   ExampleSpecFree(newBN->spec);
   /* HERE maybe I should clone the example spec? */
   newBN->spec = bn->spec;

   for(i = 0 ; i < BNGetNumNodes(bn) ; i++) {
      /* put placeholder nodes in the new network */
      VLAppend(newBN->nodes, 0);
   }

   for(i = 0 ; i < BNGetNumNodes(bn) ; i++) {
      bnn = BNGetNodeByElimOrder(bn, i);
      VLSet(newBN->nodes, BNNodeGetID(bnn), _BNNodeCloneNoCPT(bnn, newBN));
   }

   if(bn->topoSort != 0) {
      newBN->topoSort = VLNew();
      for(i = 0 ; i < VLLength(bn->topoSort) ; i++) {
         oldBnn = VLIndex(bn->topoSort, i);
         VLAppend(newBN->topoSort, VLIndex(newBN->nodes, BNNodeGetID(oldBnn)));
      }
   }

   newBN->hasCycle = bn->hasCycle;

   return newBN;
}


BeliefNet BNNewFromSpec(ExampleSpecPtr es) {
   BeliefNet bn = BNNew();
   BeliefNetNode bnn;
   int i, j;

   for(i = 0 ; i < ExampleSpecGetNumAttributes(es) ; i++) {
      bnn = BNNewNode(bn, ExampleSpecGetAttributeName(es, i));
      for(j = 0 ; j < ExampleSpecGetAttributeValueCount(es, i) ; j++) {
         BNNodeAddValue(bnn, ExampleSpecGetAttributeValueName(es, i, j));
      }
      BNNodeInitCPT(bnn);
   }

   return bn;
}

int BNStructureEqual(BeliefNet bn, BeliefNet otherNet) {
   int i;
   BeliefNetNode bnn, otherNode;

   for(i = 0 ; i < VLLength(bn->nodes) ; i++) {
      bnn = VLIndex(bn->nodes, i);
      otherNode = VLIndex(otherNet->nodes, i);

      if(!BNNodeStructureEqual(bnn, otherNode)) {
         return 0;
      }
   }

   return 1;
}

int BNGetSimStructureDifference(BeliefNet bn, BeliefNet otherNet) {
   int difference = 0;
   int i, j;
   BeliefNetNode bnn, otherBnn;

   for(i = 0 ; i < BNGetNumNodes(bn) ; i++) {

      bnn = BNGetNodeByID(bn, i);
      otherBnn = BNGetNodeByID(otherNet, i);

      for(j = 0 ; j < BNNodeGetNumParents(bnn) ; j++) {
         if(!BNNodeHasParentID(otherBnn, BNNodeGetParentID(bnn, j))) {
            difference++;
         }
      }

      for(j = 0 ; j < BNNodeGetNumParents(otherBnn) ; j++) {
         if(!BNNodeHasParentID(bnn, BNNodeGetParentID(otherBnn, j))) {
            difference++;
         }
      }
   }

   return difference;
}

void BNSetName(BeliefNet bn, char *name) {
   if(bn->name != 0) {
      MFreePtr(bn->name);
   }

   bn->name = name;
}

ExampleSpec *BNGetExampleSpec(BeliefNet bn) {
   return bn->spec;
}

BeliefNetNode BNNewNode(BeliefNet bn, char *nodeName) {
   BeliefNetNode bnn = _BNNodeNew(bn);
   AttributeSpecPtr as = AttributeSpecNew();
   AttributeSpecSetName(as, nodeName);
   AttributeSpecSetType(as, asDiscreteNamed);

   ExampleSpecAddAttributeSpec(bn->spec, as);

   _BNNodeSetIdentity(bnn, bn->spec, 
              ExampleSpecGetNumAttributes(bn->spec) - 1);

   VLAppend(bn->nodes, bnn);

   return bnn;
}

BeliefNetNode BNLookupNode(BeliefNet bn, char *name) {
   int index;

   index = ExampleSpecLookupAttributeName(bn->spec, name);

   if(index != -1) {
      return VLIndex(bn->nodes, index);
   } else {
      return 0;
   }
}

BeliefNetNode BNGetNodeByID(BeliefNet bn, int id) {
   return VLIndex(bn->nodes, id);
}

int BNGetNumNodes(BeliefNet bn) {
   return VLLength(bn->nodes);
}

static void _TopoSortCycleCheck(BeliefNet bn) {
   /* EFF HERE using the wrong data structure, should be a queue,
         because of this am n^2 instead of n */
   int i;
   VoidListPtr queue = VLNew();
   BeliefNetNode bnn, child;

   /* check that there isn't already a topo sort, and
          if there is one free it */
   if(bn->topoSort != 0) {
      VLFree(bn->topoSort);
   }

   bn->topoSort = VLNew();

   for(i = 0 ; i < VLLength(bn->nodes) ; i++) {
      bnn = VLIndex(bn->nodes, i);

      bnn->tmpInternalData = (void *)BNNodeGetNumParents(bnn);

      if(BNNodeGetNumParents(bnn) == 0) {
         VLAppend(queue, bnn);
      }
   }

   while(VLLength(queue) > 0) {
      bnn = VLRemove(queue, 0);
      VLAppend(bn->topoSort, bnn);

      for(i = 0 ; i < VLLength(bnn->childrenIDs) ; i++) {
         child = BNGetNodeByID(bn, (int)VLIndex(bnn->childrenIDs, i));
         if((int)(child->tmpInternalData) > 0) {
            child->tmpInternalData--;
            if((int)(child->tmpInternalData) == 0) {
               VLAppend(queue, child);
            }
         }
      }
   }

   if(VLLength(bn->topoSort) != VLLength(bn->nodes)) {
      /* the topo sort didn't use up every node in the net, there
            must be a cycle */
      bn->hasCycle = 1;
      VLFree(bn->topoSort);
      bn->topoSort = 0;
   } else {
      bn->hasCycle = 0;
   }

   VLFree(queue);
}

BeliefNetNode BNGetNodeByElimOrder(BeliefNet bn, int index) {
   if(bn->topoSort == 0 && (bn->hasCycle != 1)) {
      /* no cached sort, do it now */
      _TopoSortCycleCheck(bn);
   }

   if(!bn->hasCycle) {
      return VLIndex(bn->topoSort, index);
   } else {
      return 0;
   }
}

int BNHasCycle(BeliefNet bn) {
   if(bn->hasCycle == -1) {
      /* no cached value, do it now */
      _TopoSortCycleCheck(bn);
   }

   return bn->hasCycle;
}

void BNFlushStructureCache(BeliefNet bn) { 
   bn->hasCycle = -1;
   if(bn->topoSort != 0) {
      VLFree(bn->topoSort);
      bn->topoSort = 0;
   }
}

void BNZeroCPTs(BeliefNet bn) {
   int i;

   for(i = 0 ; i < VLLength(bn->nodes) ; i++) {
      BNNodeZeroCPT((BeliefNetNode)VLIndex(bn->nodes, i));
   }
}

void BNAddSample(BeliefNet bn, ExamplePtr e) {
   BNAddFractionalSample(bn, e, 1.0);
}
void BNAddSamples(BeliefNet bn, VoidListPtr samples) {
   BNAddFractionalSamples(bn, samples, 1.0);
}

void BNAddFractionalSample(BeliefNet bn, ExamplePtr e, float weight) {
   int i;

   for(i = 0 ; i < VLLength(bn->nodes) ; i++) {
      BNNodeAddFractionalSample((BeliefNetNode)VLIndex(bn->nodes, i), e,
                                                          weight);
   }
}

void BNAddFractionalSamples(BeliefNet bn, VoidListPtr samples, float weight) {
   int i;

   for(i = 0 ; i < VLLength(bn->nodes) ; i++) {
      BNNodeAddFractionalSamples((BeliefNetNode)VLIndex(bn->nodes, i), samples,
                                                          weight);
   }
}



float BNGetLogLikelihood(BeliefNet bn, ExamplePtr e) {
   int i;
   float logLikelihood;
   float prob;
   BeliefNetNode bnn;

   logLikelihood = 0;

   for(i = 0 ; i < BNGetNumNodes(bn) ; i++) {
      bnn = BNGetNodeByElimOrder(bn, i);

      /* HERE if any values are missing this will do the wrong thing */
      DebugWarn(ExampleIsAttributeUnknown(e, BNNodeGetID(bnn)),
             "Getting likelihood but an attribute is unknown.");

      prob = BNNodeGetCP(bnn, e);
      logLikelihood += log(prob);
   }
   
   return logLikelihood;
}


long BNGetNumIndependentParameters(BeliefNet bn) {
   long i, count;
   BeliefNetNode bnn;

   count = 0;

   for(i = 0 ; i < BNGetNumNodes(bn) ; i++) {
      bnn = BNGetNodeByID(bn, i);

      count += BNNodeGetNumCPTRows(bnn) * (BNNodeGetNumValues(bnn) - 1);
   }

   return count;
}

long BNGetNumParameters(BeliefNet bn) {
   long i, count;
   BeliefNetNode bnn;

   count = 0;

   for(i = 0 ; i < BNGetNumNodes(bn) ; i++) {
      bnn = BNGetNodeByID(bn, i);

      count += BNNodeGetNumCPTRows(bnn) * (BNNodeGetNumValues(bnn));
   }

   return count;
}

long BNGetMaxNodeParameters(BeliefNet bn) {
   long i, max, current;
   BeliefNetNode bnn;

   max = 0;

   for(i = 0 ; i < BNGetNumNodes(bn) ; i++) {
      bnn = BNGetNodeByID(bn, i);
      current = BNNodeGetNumCPTRows(bnn) * (BNNodeGetNumValues(bnn));
      if(current > max) {
         max = current;
      }
   }

   return max;
}


BeliefNet BNInitLikelihoodSampling(BeliefNet bn, ExamplePtr e) {
   BeliefNet newNet = BNClone(bn);
   BNZeroCPTs(newNet);
   return newNet;
}

// mustbeallknown: 0=unknowns are ok, 1=unknowns are not okay
void CheckExample(ExamplePtr e, int mustBeAllKnown) {
  int i;
  for (i=0; i<ExampleGetNumAttributes(e); i++) {
    if (ExampleIsAttributeUnknown(e, i)) {
      if (mustBeAllKnown)
        printf("Error! Unkown attribute value here\n");
    }
    else
    {
      int val = ExampleGetDiscreteAttributeValueUnsafe(e, i);
      if (val < 0)
        printf("Error! Negative attribute value here\n");
    }
  }
}

int _zeroWeights;
int _totalSamples;
void BNAddLikelihoodSamples(BeliefNet bn, BeliefNet newNet, ExamplePtr e, int numSamples) {
   BeliefNetNode bnn;
   int i, j, attNum;
   double sampleWeight;
   ExamplePtr sample;

   //CheckExample(e, 0);

   for(i = 0 ; i < numSamples ; i++) {
      sampleWeight = 1;
      sample = ExampleNew(bn->spec);
      for(j = 0 ; j < BNGetNumNodes(bn) ; j++) {
         bnn = BNGetNodeByElimOrder(bn, j);
         attNum = BNNodeGetID(bnn);

         if(ExampleIsAttributeUnknown(e, attNum)) {
            BNNodeSetRandomValueGivenParents(bnn, sample);
            assert(ExampleGetDiscreteAttributeValue(sample, attNum) >= 0);
         } else {
            ExampleSetDiscreteAttributeValue(sample, attNum,
                     ExampleGetDiscreteAttributeValueUnsafe(e, attNum));
            sampleWeight *= BNNodeGetCP(bnn, sample);
         }
      }
      BNAddFractionalSample(newNet, sample, sampleWeight);
      ExampleFree(sample);

      _totalSamples++;
      if(sampleWeight == 0) {
         _zeroWeights++;
      }
   }
}


BeliefNet BNLikelihoodSampleNTimes(BeliefNet bn, ExamplePtr e, int numSamples) {
  BeliefNet newNet = BNInitLikelihoodSampling(bn, e);
  BNAddLikelihoodSamples(bn, newNet, e, numSamples);

  printf("total %d zeroes %d that's %f%%\n", _totalSamples, _zeroWeights, (float)_zeroWeights / (float)_totalSamples);

  return newNet;
}


ExamplePtr BNGenerateSample(BeliefNet bn) {
   BeliefNetNode bnn;
   ExamplePtr e = ExampleNew(BNGetExampleSpec(bn));
   int i;

   ExampleSetClass(e, 0);

   for(i = 0 ; i < BNGetNumNodes(bn) ; i++) {
      bnn = BNGetNodeByElimOrder(bn, i);
      BNNodeSetRandomValueGivenParents(bnn, e);
   }

   return e;
}

void BNSetPriorStrength(BeliefNet bn, double strength) {
   int i;
   BeliefNetNode bnn;

   for(i = 0 ; i < BNGetNumNodes(bn) ; i++) {
      bnn = BNGetNodeByID(bn, i);
      BNNodeSetPriorStrength(bnn, strength);
   }
}

void BNSmoothProbabilities(BeliefNet bn, double strength) {
   int i;
   BeliefNetNode bnn;

   for(i = 0 ; i < BNGetNumNodes(bn) ; i++) {
      bnn = BNGetNodeByID(bn, i);
      BNNodeSmoothProbabilities(bnn, strength);
   }
}

void BNSetUserData(BeliefNet bn, void *data) {
   bn->userData = data;
}

//void *BNGetUserData(BeliefNet bn) {
//   return bn->userData;
//}

BeliefNet BIFParse(FILE *input);
BeliefNet BNReadBIF(char *fileName) {
   BeliefNet out;
   FILE *input;

   input = fopen(fileName, "r");
   if(input == 0) {
      return 0;
   }

   out = BIFParse(input);

   fclose(input);
   return out;
}

BeliefNet BNReadBIFFILEP(FILE *file) {
   return BIFParse(file);
}

static void _BIFWriteProbHelper(BeliefNetNode bnn, 
               ExamplePtr e, int parentIndex, FILE* out) {
   int i;
   BeliefNetNode parent;

   if(parentIndex == BNNodeGetNumParents(bnn)) {
      /* base case, write the probabilities for this parent combo */
      if(parentIndex == 0) { 
         /* no parents, write in 'table' format */
         fprintf(out, "   table ");
      } else {
         fprintf(out, "   (");
         for(i = 0 ; i < BNNodeGetNumParents(bnn) ; i++) {
            parent = BNNodeGetParent(bnn, i);
            fprintf(out, "%s ", ExampleSpecGetAttributeValueName(bnn->spec,
              BNNodeGetID(parent),
               ExampleGetDiscreteAttributeValue(e, BNNodeGetID(parent))));
         }
         fprintf(out, ")");
      }

      /* write the probabilities */
      for(i = 0 ; i < ExampleSpecGetAttributeValueCount(bnn->spec, 
                            BNNodeGetID(bnn)) ; i++) {
         ExampleSetDiscreteAttributeValue(e, BNNodeGetID(bnn), i);
         fprintf(out, " %f", BNNodeGetCP(bnn, e));
      }

      fprintf(out, ";\n");
   } else {
      /* set up for the next recursive call */
      parent = BNNodeGetParent(bnn, parentIndex);
      for(i = 0 ; i < ExampleSpecGetAttributeValueCount(parent->spec, 
                           BNNodeGetID(parent)) ; i++) {
         ExampleSetDiscreteAttributeValue(e, BNNodeGetID(parent), i);
         _BIFWriteProbHelper(bnn, e, parentIndex + 1, out);
      }
   }
}

static void _BIFWriteProbs(BeliefNetNode bnn, FILE *out) {
   ExamplePtr e;

   e = ExampleNew(bnn->spec);
   _BIFWriteProbHelper(bnn, e, 0, out);
   ExampleFree(e);

}

void BNWriteBIF(BeliefNet bn, FILE *out) {
   int i,j;
   BeliefNetNode bnn;


   if(bn->name == 0) {
      fprintf(out, "network %s { }\n", "Unknown");
   } else {
      fprintf(out, "network %s { }\n", bn->name);
   }
   

   for(i = 0 ; i < ExampleSpecGetNumAttributes(bn->spec) ; i++) {
      fprintf(out, 
           "variable %s {\n", ExampleSpecGetAttributeName(bn->spec, i));

      fprintf(out, "   type discrete [ %d ] { ", 
                ExampleSpecGetAttributeValueCount(bn->spec, i));

      for(j = 0 ; j < ExampleSpecGetAttributeValueCount(bn->spec, i) ; j++) {
         fprintf(out, "%s ", ExampleSpecGetAttributeValueName(bn->spec, i, j));
      }

      fprintf(out, "};\n");
      fprintf(out, "}\n");
   }

   /* Write out all the probabilities for nodes that have them */
   for(i = 0 ; i < ExampleSpecGetNumAttributes(bn->spec) ; i++) {
      bnn = VLIndex(bn->nodes, i);
      if(bnn->eventCounts != 0) {
         fprintf(out, "probability ( %s | ", 
            ExampleSpecGetAttributeName(bn->spec, BNNodeGetID(bnn)));

         for(j = 0 ; j < BNNodeGetNumParents(bnn) ; j++) {
            fprintf(out, "%s ", ExampleSpecGetAttributeName(bn->spec, 
                      BNNodeGetID(BNNodeGetParent(bnn, j))));
         }

         fprintf(out, ") {\n");

         _BIFWriteProbs(bnn, out);

         fprintf(out, "}\n");
      }
   }
}

void BNPrintStats(BeliefNet bn) {
   BeliefNetNode bnn;
   int i, maxValues, maxParents, numParents, numCPTEntries;
   int minParents;

   DebugMessage(1, 0, "Belief Net Stats for %s:\n", bn->name);
   DebugMessage(1, 0, "   Num Nodes:\t\t%d\n", BNGetNumNodes(bn));

   maxValues = maxParents = numParents = numCPTEntries = 0;

   minParents = 32000;

   for(i = 0 ; i < BNGetNumNodes(bn) ; i++) {
      bnn = BNGetNodeByID(bn, i);

      maxValues = max(maxValues, BNNodeGetNumValues(bnn));
      maxParents = max(maxParents, BNNodeGetNumParents(bnn));
      minParents = min(minParents, BNNodeGetNumParents(bnn));
      numParents += BNNodeGetNumParents(bnn);
      numCPTEntries +=  BNNodeGetNumCPTRows(bnn) * BNNodeGetNumValues(bnn);
   }

   DebugMessage(1, 0, "   Max Values:\t\t%d\n", maxValues);
   DebugMessage(1, 0, "   Num Parents:\t\t%d\n", numParents);
   DebugMessage(1, 0, "   Min Parents:\t\t%d\n", minParents);
   DebugMessage(1, 0, "   Max Parents:\t\t%d\n", maxParents);
   DebugMessage(1, 0, "   Avg Parents:\t\t%.3f\n", (float)numParents / 
                                   (float)BNGetNumNodes(bn));
   DebugMessage(1, 0, "   Num CPT Entries:\t%d\n", numCPTEntries);
   DebugMessage(1, 0, "   Max CPT Entries:\t%d\n", BNGetMaxNodeParameters(bn));
}
/*****************************************
       mattr added below and in .h
       move to where they belong later
        if approved
******************************************/







/************************************************
    Used by the things I added above
    *******************************************/

/* Added by mattr. Part of the hacked up version. Come back to this later */
float _BNNodeCPTGetCPIndexed(BeliefNetNode bnn, int row, int value) {
   if(bnn->rowCounts[row] == 0) {
      /* HERE is this the right thing? */
      return 0;
   }

   return bnn->eventCounts[row][value] / bnn->rowCounts[row];
}

/* like converting from a eventRowIndex back to an ExamplePtr and then returning
   the value of the particular parent. For efficiency we just return once we
   know one parent value
*/
static int _FindParentValue(BeliefNetNode bnn, int eventRowIndex, int parentIndex) {
   int i, offset;
   BeliefNetNode parent;

   //if (HasIndependentParents(bnn)) {
   //   assert(BNNodeGetNumValues(bnn) == 2); // actually assumes all parents are bi-valued
   //   return eventRowIndex/2;
   //}
   //else {
     // First, find the parent's ofset.
     // OPTIMIZE: cache this
     offset=1;
     for (i = VLLength(bnn->parentIDs)-1; i>parentIndex ; i--) {
       parent = VLIndex(bnn->parentIDs, i);
       offset *= BNNodeGetNumValues(parent);
     }
     // The value is the (row/offset) % numvalues (I think!)
     return (eventRowIndex/offset) % BNNodeGetNumValues(VLIndex(bnn->parentIDs, parentIndex));
   //}
}



// "Subtracts" the current example from the cpt before returning the prob
float _BNNodeGetCPMinusThisExample(BeliefNetNode bnn, ExamplePtr e) {
   int rowIndex = _FindEventRowIndex(bnn, e);
   float *eventRow;
   float event, rowSum;

   //printf("rowIndex %d\n", rowIndex);
   if(rowIndex != -1) {
      //assert(rowIndex >= 0);
      eventRow = bnn->eventCounts[rowIndex];
      //if(!ExampleIsAttributeUnknown(e, BNNodeGetID(bnn))) {
         rowSum = bnn->rowCounts[rowIndex];
         event = 
            eventRow[ExampleGetDiscreteAttributeValue(e, BNNodeGetID(bnn))];

         //printf("event %f rowSum %f\n", event, rowSum);

         if(rowSum == 0) {
            /* HERE is this the right thing? */
            return 0;
         }

         return (event-1) / (rowSum-1);
      //}
   }

   return -1;
}



/******************
  done with used by...
  *****************/










void BNNodeAllocateCPT(BeliefNetNode bnn, float ***eventCounts, float **rowCounts)
{
  int numParentCombinations, numValues;
  int i;
  numParentCombinations = _GetNumParentCombinations(bnn);
  numValues = BNNodeGetNumValues(bnn);

  *eventCounts = MNewPtr(sizeof(float*) * numParentCombinations);
  *rowCounts = MNewPtr(sizeof(float) * numParentCombinations);
  for (i=0; i<numParentCombinations; i++) 
    (*eventCounts)[i] = MNewPtr(sizeof(float)*numValues);
}


void BNNodeCopyCPT(BeliefNetNode bnn, float ***eventCounts, float **rowCounts)
{
  int numParentCombinations, numValues;
  int i,j;
  numParentCombinations = _GetNumParentCombinations(bnn);
  numValues = BNNodeGetNumValues(bnn);
  if (bnn->eventCounts == 0) {
    *eventCounts = NULL;
    *rowCounts = NULL;
    return;
  }
  BNNodeAllocateCPT(bnn, eventCounts, rowCounts);
  for (i=0; i<numParentCombinations; i++) {
    (*rowCounts)[i] = bnn->rowCounts[i];
    for (j=0; j<numValues; j++) {
      (*eventCounts)[i][j] = bnn->eventCounts[i][j];
    }
  }
}

void BNNodeRestoreCPT(BeliefNetNode bnn, float **eventCounts, float *rowCounts)
{
  int numParentCombinations, numValues;
  int i,j;
  numParentCombinations = _GetNumParentCombinations(bnn);
  numValues = BNNodeGetNumValues(bnn);
  if (eventCounts == 0)
    return;
  for (i=0; i<numParentCombinations; i++) {
    bnn->rowCounts[i] = rowCounts[i];
    for (j=0; j<numValues; j++) {
      bnn->eventCounts[i][j] = eventCounts[i][j];
    }
  }
}

void BNNodeFreeCopiedCPT(BeliefNetNode bnn, float **eventCounts, float *rowCounts)
{
  int numParentCombinations, i;
  numParentCombinations = _GetNumParentCombinations(bnn);
  for (i=0; i<numParentCombinations; i++)
    MFreePtr(eventCounts[i]);
  MFreePtr(rowCounts);
  MFreePtr(eventCounts);
}
  
void BNNodeRemoveParentByID(BeliefNetNode bnn, int parentID) {
   int index;
   index = BNNodeLookupParentIndexByID(bnn, parentID);
   if (index == -1)
      return;         // No edge there. error
   BNNodeRemoveParent(bnn, index);
}   

void BNNodeRemoveEdge(BeliefNetNode parent, BeliefNetNode child) {
   int index;
   index = BNNodeLookupParentIndex(child, parent);
   if (index == -1)
      return;         // No edge there. error
   BNNodeRemoveParent(child, index);
}


// Copied from vfbn.c::_UpdateNodeBD
//  and modified, hopefully optimized a little too
double _Shrink_GetBDScore(BeliefNetNode bnn)
{
  int numCPTRows, numSamples, numValues, j,k;
  double avgDataLL, gamma1, gamma_numValues;
  numCPTRows = BNNodeGetNumCPTRows(bnn);
  numSamples = BNNodeGetNumSamples(bnn);
  numValues = BNNodeGetNumValues(bnn);

  avgDataLL = 0;
  gamma1 = StatLogGamma(1);
  gamma_numValues = StatLogGamma(numValues);
  for (j=0; j<numCPTRows; j++) {
    avgDataLL += (gamma_numValues -
                 StatLogGamma(numValues + bnn->rowCounts[j]));
    for (k=0; k<numValues; k++)
      avgDataLL += StatLogGamma(1 + bnn->eventCounts[j][k]) - gamma1;
  }
  /* now scale it by the likelihood of the node given structural prior */
  //data->avgDataLL += _GetStructuralDifferenceScoreNode(bnn);
  return avgDataLL;
}

void _Shrink_AddData(BeliefNetNode bnn, VoidListPtr data)
{
  int i;
  for (i=0; i<VLLength(data); i++)
    BNNodeAddFractionalSample(bnn, VLIndex(data, i), 1.0);
}

// Fills in the list of nodes. Each node has a subset of parents (0 parents, 
//  1 parent, etc...). The parents are added in the order that has the 
//  highest BD score at each step
void _Shrink_GetIncreasinglyDetailedNodes(BeliefNetNode bnn, VoidListPtr data, VoidListPtr nodes, int stopAtNParents) {
  int numParents, maxParents, i, bestParent;
  VoidListPtr unassignedParents;
  BeliefNetNode nodeSoFar, bestNode;
  double bestScore;
  
  maxParents = BNNodeGetNumParents(bnn);
  unassignedParents = VLNew();
  for (i=0; i<maxParents; i++)
    VLAppend(unassignedParents, (void*)BNNodeGetParent(bnn, i));

  // Start by preparing the node with no parents, and adding it to the
  //  list of nodes
  nodeSoFar = _BNNodeCloneNoCPT(bnn, bnn->bn);
  VLFree(nodeSoFar->parentIDs);
  nodeSoFar->parentIDs = VLNew();
  BNNodeInitCPT(nodeSoFar);
  _Shrink_AddData(nodeSoFar, data);
  VLAppend(nodes, nodeSoFar);

  // Now, for each number of parents 1-maxParents, find the best
  //  node and add it to the list of nodes as well
  for (numParents = 1; numParents <= stopAtNParents; numParents++)
  {
    // Create all nodes with 'numparents' parents, where the first
    // numparents-1 are all the same set, and the last is one of the 
    // remaining ones
    bestScore = 0;
    bestNode = NULL;
    bestParent = 0;
    for (i=0; i<VLLength(unassignedParents); i++)
    {
      double score;
      BeliefNetNode newNode = _BNNodeCloneNoCPT(nodeSoFar, nodeSoFar->bn);
      BeliefNetNode parent = VLIndex(unassignedParents, i);
      // Add in the parent
      VLAppend(newNode->parentIDs, (void *)parent->attributeID);
      //newNode->numParentCombinations = -1;
      BNNodeInitCPT(newNode);
      _Shrink_AddData(newNode, data);
      score = _Shrink_GetBDScore(newNode);
      //printf("%p: With %d parents: parent %d score=%g\n",bnn, numParents,i,score);
      if (score > bestScore || bestNode == NULL) {
        if (bestNode != NULL)
          BNNodeFree(bestNode);
        bestScore = score;
        bestNode = newNode;
        bestParent = i;
      }
      else
        BNNodeFree(newNode);
    }
    VLAppend(nodes, bestNode);
    VLRemove(unassignedParents, bestParent);
    nodeSoFar=bestNode;

    // The alternative to the above is to collapse the already-counted CPTs
    //  in a smart way. That would be faster when there is lots of data
    //  but probably slower when there is little data
  }
}
    
void _Shrink_doShrinking(BeliefNetNode bnn, VoidListPtr data, VoidListPtr nodeslist, int iterations, int doDirichlet)
{
  int iter, numMixtures, i, j;
  double totalP, totalBeta;
  double *mixtureWeight, *beta, *p;
  BeliefNetNode *nodes;
  double **CPcache;       // CPcache[example][mixture]

  numMixtures = VLLength(nodeslist);
  mixtureWeight = MNewPtr(sizeof(double)*numMixtures);
  beta = MNewPtr(sizeof(double)*numMixtures);
  p = MNewPtr(sizeof(double)*numMixtures);
  nodes = MNewPtr(sizeof(BeliefNetNode)*numMixtures);

  for (i=0; i<numMixtures; i++) {
    mixtureWeight[i] = 1.0/numMixtures;
    BNNodeSmoothProbabilities( VLIndex(nodeslist, i), 1.0 );
    nodes[i] = VLIndex(nodeslist, i);
  }

  CPcache = MNewPtr(sizeof(double*)*VLLength(data));
  for (i=0; i<VLLength(data); i++) {
    CPcache[i] = MNewPtr(sizeof(double)*numMixtures);
    for (j=0; j<numMixtures; j++)
      CPcache[i][j] = _BNNodeGetCPMinusThisExample(nodes[j], VLIndex(data,i));
  }

  for (iter=0; iter<iterations; iter++)
  {
    for (i=0; i<numMixtures; i++)
      beta[i] = 0.0;

    for (j=0; j<VLLength(data); j++) {
      //ExamplePtr e = VLIndex(data,j);
      totalP=0;
      for (i=0; i<numMixtures; i++) {
        //p[i] = mixtureWeight[i] * _BNNodeGetCPMinusThisExample(nodes[i], e);
        p[i] = mixtureWeight[i] * CPcache[j][i];
        totalP += p[i];
      }
      for (i=0; i<numMixtures; i++)
        beta[i] += p[i] / totalP;
    }

    // mixture weights = normalized beta
    totalBeta=0;
    for (i=0; i<numMixtures; i++)
      totalBeta += beta[i];
    for (i=0; i<numMixtures; i++)
      mixtureWeight[i] = beta[i] / totalBeta;
    //printf("%p: iter %d, weights: ", bnn, iter);
    //for (i=0; i<numMixtures; i++)
    //  printf("%g ", mixtureWeight[i]);
    //printf("\n");
  }
  for (i=0; i<VLLength(data); i++)
    MFreePtr(CPcache[i]);
  MFreePtr(CPcache);

  // Okay, create a new CPT which is a merged version of the
  //  node cpts
  {
    int value, numValues, numParentCombinations, row, totalSkip;
    int *skiprows;
    BeliefNetNode fullNode;
    int numMixturesToUse;

    numValues = BNNodeGetNumValues(bnn);
    fullNode = nodes[numMixtures-1];
    // Copy over the parents array so it is in the same order as mixtures
    for (i=0; i<BNNodeGetNumParents(bnn); i++)
      VLSet(bnn->parentIDs, i, VLIndex(fullNode->parentIDs,i));
    numParentCombinations = _GetNumParentCombinations(bnn);
    
    // WARNING: This breaks the BN ADT
    // WARNING: This only works because each node adds just one parent and
    //   they all keep the same parent order
    skiprows = MNewPtr(sizeof(int)*numMixtures);
    totalSkip=1;
    for (i=numMixtures-1; i>=0; i--) {
      // This is the parent that the ith mixture added
      skiprows[i]=totalSkip;
      if (i > 0) 
        totalSkip *= BNNodeGetNumValues( BNNodeGetParent(bnn, i-1));
    }

    if (doDirichlet) numMixturesToUse = numMixtures-1;
    else numMixturesToUse = numMixtures;

    for (row=0; row < numParentCombinations; row++) {
      bnn->rowCounts[row] = fullNode->rowCounts[row];
      for (value=0; value<numValues; value++) {
        // Calculate p(mixture)
        double pmix=0;
        for (i=0; i<numMixturesToUse; i++) {
          int mixrow = row/skiprows[i];
          pmix += mixtureWeight[i] * nodes[i]->eventCounts[mixrow][value] / nodes[i]->rowCounts[mixrow];
        }
        bnn->eventCounts[row][value] = bnn->rowCounts[row] * pmix;
      }
    }

    // If dirichlet, we need to recalculate the row totals
    // OPTIMIZE: There may be a closed form expression for this instead
    if (doDirichlet) {
      for (row=0; row<numParentCombinations; row++) {
        bnn->rowCounts[row]=0;
        for (value=0; value<numValues; value++) {
          bnn->eventCounts[row][value] /= mixtureWeight[numMixtures-1];
          bnn->eventCounts[row][value]+=1;
          bnn->rowCounts[row] += bnn->eventCounts[row][value];
        }
      }
    }

    MFreePtr(skiprows);
  }

  MFreePtr(mixtureWeight);
  MFreePtr(beta);
  MFreePtr(p);
  MFreePtr(nodes);
  
}


void BNNodeShrink(BeliefNetNode bnn, VoidListPtr data, int doDirichlet) {
  // Not neccessarily the most efficient way to do this, but when
  //  there is little data (e.g. 10 instances, 100 instances), it is
  //  probably faster to do this (loop through data) rather than
  //  figuring out how to collapse the CPTs
  int i;

  VoidListPtr nodes = VLNew();
  _Shrink_GetIncreasinglyDetailedNodes(bnn, data, nodes, BNNodeGetNumParents(bnn));
  _Shrink_doShrinking(bnn,data,nodes,20, doDirichlet);
  for (i=0; i<VLLength(nodes); i++)
    BNNodeFree( VLIndex(nodes, i) );

  VLFree(nodes);
}

void BNNodeTruncateToNParents(BeliefNetNode bnn, VoidListPtr data, int maxParents) {
  BeliefNetNode goodParentNode;
  int i;
  VoidListPtr nodes = VLNew();
  if (BNNodeGetNumParents(bnn) <= maxParents)
    return;

  _Shrink_GetIncreasinglyDetailedNodes(bnn, data, nodes, maxParents);
  // The last one of the nodes array is the node with maxParents number
  //  of parents
  goodParentNode = VLIndex(nodes, VLLength(nodes)-1);
  for (i=0; i<BNNodeGetNumParents(bnn); i++) {
    if (!BNNodeHasParentID(goodParentNode, BNNodeGetParentID(bnn, i))) {
      // Parent not in set of good parents, so remove from here
      BNNodeRemoveParent(bnn, i);
      i--;  // We just removed the one at i, so don't move forward one
    }
  }
}



void BNRemoveNode(BeliefNet bn, BeliefNetNode node) {
   int i;
   int nodeid = BNNodeGetID(node);

   // First, remove all the edges to and from the node 
   //  OPTIMZE: would be faster not to update cpt over 
   //  and over when removing parents 
   for (i=0; i<VLLength(node->parentIDs); i++)
      BNNodeRemoveParentByID(node, (int)VLIndex(node->parentIDs, i));
   for (i=0; i<VLLength(node->childrenIDs); i++) {
      BeliefNetNode child = BNGetNodeByID(bn, (int)VLIndex(node->childrenIDs, i));
      BNNodeRemoveParentByID(child, nodeid);
   }

   // Set examplespec to ignore this node
   ExampleSpecIgnoreAttribute(BNGetExampleSpec(bn), nodeid);

   // Free memory and set node to null so we catch accidental access to it
   // ERROR: not done. I (mattr) didn't feel like going through all
   //  the code and checking for nulls everywhere. So, if you remove a node
   //  you better not do much with the net other than save it
   // ERROR: todo: Invalidate topo order, update all funcs to check for ignored/null
}


// p is already allocated to be the number of values that bnn can take
void BNNodeCPTGetP(BeliefNetNode bnn, ExamplePtr e, double *p)
{
  int i;
  float *eventRow;
  int rowIndex = _FindEventRowIndex(bnn, e);
  //assert(!HasIndependentParents(bnn));
  if (rowIndex != -1) {
    eventRow = bnn->eventCounts[rowIndex];
    for (i=0; i<BNNodeGetNumValues(bnn); i++)
    {
      if (bnn->numSamples > 0)
        p[i] = eventRow[i] / bnn->numSamples;
      else
        p[i] = 0;
    }
  }
}

double BNNodeCPTGetCP(BeliefNetNode bnn, ExamplePtr e) {
   int rowIndex = _FindEventRowIndex(bnn, e);
   //assert(!HasIndependentParents(bnn));
   if(rowIndex != -1) {
      if(!ExampleIsAttributeUnknown(e, BNNodeGetID(bnn))) {
         return _BNNodeCPTGetCPIndexed(bnn, rowIndex,
               ExampleGetDiscreteAttributeValue(e, BNNodeGetID(bnn)));
      }
   }
   return -1;
}


// fills in p = P(parent,child)
void BNNodeGetJointProb(BeliefNetNode parent, BeliefNetNode child, double p[2][2])
{
  //if (HasIndependentParents(child))
  //{
  //  int parentIndex = BNNodeLookupParentIndex(child, parent);
  //  assert(parentIndex != -1);
  //  BNNodeGetJointProbByParent(child, parentIndex, p);
  //}
  //else {
    float counts[2][2] = { {0,0}, {0,0} };
    float countsTotal=0;
    int row;
    //float rowTotal;
    int numRows = _GetNumParentCombinations(child);
    //int numVals = 2;
    int parentIndex = BNNodeLookupParentIndex(child,parent);
    assert(parentIndex >= 0);
    assert(BNNodeGetNumValues(parent) == 2);
    assert(BNNodeGetNumValues(child) == 2);

    for(row = 0 ; row < numRows ; row++) {
      // OPTIMIZE: cache offset
      int parentValue = _FindParentValue(child, row, parentIndex);
      counts[parentValue][0] += child->eventCounts[row][0];
      counts[parentValue][1] += child->eventCounts[row][1];
    }
    countsTotal = counts[0][0] + counts[0][1] + counts[1][0] + counts[1][1];
    if (countsTotal == 0) {
      printf("error, Countstotal=0");
      p[0][0] = 0.25;
      p[0][1] = 0.25;
      p[1][0] = 0.25;
      p[1][1] = 0.25;
     return;
    }
    else {
      p[0][0] = counts[0][0] / countsTotal;
      p[0][1] = counts[0][1] / countsTotal;
      p[1][0] = counts[1][0] / countsTotal;
      p[1][1] = counts[1][1] / countsTotal;
    }
  //}


}





