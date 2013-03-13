#include "REPrune.h"

typedef struct _PRUNEDATA_ {
   long errors; /* errors made by this node */
   int class;
   long errorDelta; /* change in errors by pruning to this node */
   long seen;
   DecisionTreePtr parent;
} PruneData, *PruneDataPtr;


static void _InitPruneData(DecisionTreePtr current, DecisionTreePtr parent) {
   PruneDataPtr data;
   long i;

   data = MNewPtr(sizeof(PruneData));
   data->errors = 0;
   data->class = DecisionTreeGetMostCommonClass(current);
   data->errorDelta = 0;
   data->seen = 0;
   data->parent = parent;

   DecisionTreeSetGrowingData(current, data);
   if(current->nodeType != dtnGrowing && current->nodeType != dtnLeaf) {
      for(i = 0 ; i < VALLength(current->children) ; i++) {
         _InitPruneData(VALIndex(current->children, i), current);
      }
   }
}

static void _BatchUpdateErrorCounts(DecisionTreePtr current, ExamplePtr e) {
   PruneDataPtr data = DecisionTreeGetGrowingData(current);


   if(data->class != ExampleGetClass(e)) {
      data->errors += 1;
      data->errorDelta += 1;
   }

   if(current->nodeType != dtnGrowing && current->nodeType != dtnLeaf) {
      _BatchUpdateErrorCounts(DecisionTreeOneStepClassify(current, e), e);
   }
}

static long _BatchInitErrorDelta(DecisionTreePtr current) {
   PruneDataPtr data = DecisionTreeGetGrowingData(current);
   long sum = 0;
   long i;

   if(current->nodeType != dtnGrowing && current->nodeType != dtnLeaf) {
      for(i = 0 ; i < VALLength(current->children) ; i++) {
         sum += _BatchInitErrorDelta(VALIndex(current->children, i));
      }

      data->errorDelta -= sum;
      return sum;
   } else {
      return data->errors;
   }
}

static void _FreePruneData(DecisionTreePtr dt) {
   long i;

   MFreePtr(DecisionTreeGetGrowingData(dt));
   if(dt->nodeType != dtnGrowing && dt->nodeType != dtnLeaf) {
      for(i = 0 ; i < VALLength(dt->children) ; i++) {
         _FreePruneData(VALIndex(dt->children, i));
      }
   }
}

static DecisionTreePtr _BatchFindBestPruneNode(DecisionTreePtr dt) {
   PruneDataPtr bestData = DecisionTreeGetGrowingData(dt);
   DecisionTreePtr best = dt;
   PruneDataPtr tmpData;
   DecisionTreePtr tmp;
   long i;

   if(dt->nodeType != dtnGrowing && dt->nodeType != dtnLeaf) {
      for(i = 0 ; i < VALLength(dt->children) ; i++) {
         tmp = _BatchFindBestPruneNode(VALIndex(dt->children, i));
         if(tmp != 0) {
            tmpData = DecisionTreeGetGrowingData(tmp);
            if(tmpData->errorDelta < bestData->errorDelta) {
               best = tmp;
               bestData = tmpData;
            }
         }
      }

      if(bestData->errorDelta <= 0) {
         return best;
      }
   }

   return 0;
}

static void _PrintPruneData(DecisionTreePtr dt, int level) {
   PruneDataPtr bestData = DecisionTreeGetGrowingData(dt);
   long i;

   for(i = 0 ; i < level ; i++) {
      printf(" ");
   }
   printf("l%d e%ld d%ld c%d\n", level, 
        bestData->errors, bestData->errorDelta, bestData->class);

   if(dt->nodeType != dtnGrowing && dt->nodeType != dtnLeaf) {
      for(i = 0 ; i < VALLength(dt->children) ; i++) {
         _PrintPruneData(VALIndex(dt->children, i), level + 1);
      }
   }
}

static void _BatchPruneNode(DecisionTreePtr dt) {
   int i;
   PruneDataPtr data = DecisionTreeGetGrowingData(dt);
   DecisionTreePtr current;
   PruneDataPtr currentData;

   /* update parent delta errors */
   current = data->parent;
   while(current != 0) {
      currentData = DecisionTreeGetGrowingData(current);
      /* this seems backwards, but by improving errors below the parent
           looks like a less good place to prune */
      currentData->errorDelta -= data->errorDelta;
      current = currentData->parent;
   }

   /* free stuff below the node */
   if(dt->nodeType != dtnGrowing && dt->nodeType != dtnLeaf) {
      for(i = 0 ; i < VALLength(dt->children) ; i++) {
         current = VALIndex(dt->children, i);
         _FreePruneData(current);
      }
   }

   /* update this node */
   DecisionTreeSetTypeLeaf(dt);
   DecisionTreeSetClass(dt, data->class);

   data->errorDelta = data->errors;
}

void REPruneBatch(DecisionTreePtr dt, VoidAListPtr examples) {
   long i;
   int progress;
   DecisionTreePtr pruneNode;

   // propogate the prune data over the whole tree
   _InitPruneData(dt, 0);

   // pass the prune set through the tree recording errors
   for(i = 0 ; i < VALLength(examples) ; i++) {
      _BatchUpdateErrorCounts(dt, VALIndex(examples, i));
   }

   // collect the error deltas
   _BatchInitErrorDelta(dt);
   //_PrintPruneData(dt, 0);

   progress = 1;
   // while there is an improvement
   while(progress) {
      // find the best candidate
      pruneNode = _BatchFindBestPruneNode(dt);

      // prune the best candidate if appropriate
      if(pruneNode != 0) {
         //printf("prune a node\n");
         _BatchPruneNode(pruneNode);
      } else {
         progress = 0;
      }
   }

   // free the prune data
   _FreePruneData(dt);
}

void REPruneBatchFile(DecisionTreePtr dt, FILE *exampleIn, long pruneMax) {
   long seen;
   ExamplePtr e;
   int done, progress;
   DecisionTreePtr pruneNode;

   // propogate the prune data over the whole tree
   _InitPruneData(dt, 0);

   done = 0;
   seen = 0;
   e = ExampleRead(exampleIn, dt->spec);                                    
   while(!done && e != 0) {
      _BatchUpdateErrorCounts(dt, e);
      seen++;
      //printf("seen %ld\n", seen);
      //ExampleWrite(e, stdout);
      ExampleFree(e);
      e = ExampleRead(exampleIn, dt->spec);                                    
      if(pruneMax != 0 && seen >= pruneMax) {
         /* if we've seen enough then stop */
         done = 1;
      }
   }

   // collect the error deltas
   _BatchInitErrorDelta(dt);

   progress = 1;
   // while there is an improvement
   while(progress) {
      // find the best candidate
      pruneNode = _BatchFindBestPruneNode(dt);

      // prune the best candidate if appropriate
      if(pruneNode != 0) {
         //printf("prune a node\n");
         _BatchPruneNode(pruneNode);
      } else {
         progress = 0;
      }
   }

   // free the prune data
   _FreePruneData(dt);
}
