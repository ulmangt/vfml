#include "memory.h"
#include "Debug.h"
#include "DecisionTree.h"

DecisionTreePtr DecisionTreeNew(ExampleSpecPtr spec) {
   DecisionTreePtr dt = MNewPtr(sizeof(DecisionTree));

   dt->nodeType = dtnGrowing;
   dt->spec = spec;
   dt->growingData = 0;
   dt->myclass = 0;
   dt->children = 0;

   dt->classDistribution = MNewPtr(sizeof(float) * 
                                ExampleSpecGetNumClasses(spec));

   DecisionTreeZeroClassDistribution(dt);

   return dt;
}

void DecisionTreeFree(DecisionTreePtr dt) {
   int i;

   if(dt->nodeType == dtnDiscrete || dt->nodeType == dtnContinuous) {
      for(i = VALLength(dt->children) - 1 ; i >= 0 ; i--){
         DecisionTreeFree(VALRemove(dt->children, i));
      }
      VALFree(dt->children);
   }

   MFreePtr(dt->classDistribution);

   MFreePtr(dt);
}

DecisionTreePtr DecisionTreeClone(DecisionTreePtr dt) {
   int i;
   DecisionTreePtr clone = DecisionTreeNew(dt->spec);

   clone->nodeType = dt->nodeType;
   clone->growingData = dt->growingData;
   clone->splitAttribute = dt->splitAttribute;
   clone->splitThreshold = dt->splitThreshold;
   clone->myclass = dt->myclass;

   if(dt->nodeType == dtnDiscrete || dt->nodeType == dtnContinuous) {
      clone->children = VALNew();
      for(i = 0 ; i < VALLength(dt->children) ; i++) {
         VALAppend(clone->children,
               DecisionTreeClone(VALIndex(dt->children, i)));
      }
   }

   return clone;
}

int DecisionTreeIsLeaf(DecisionTreePtr dt) {
   return dt->nodeType == dtnLeaf;
}

int DecisionTreeIsTreeGrowing(DecisionTreePtr dt) {
   int i;

   switch(dt->nodeType) {
    case dtnLeaf:
      return 0;
      break;
    case dtnGrowing:
      return 1;
      break;
    case dtnContinuous:
    case dtnDiscrete:
      for(i = 0 ; i < VALLength(dt->children) ; i++) {
         if(DecisionTreeIsTreeGrowing(VALIndex(dt->children, i))) {
            return 1;
         }
      }
      return 0;
      break;
   }

   return 0;
}

int DecisionTreeIsNodeGrowing(DecisionTreePtr dt) {
   return dt->nodeType == dtnGrowing;
}

int DecisionTreeGetClass(DecisionTreePtr dt) {
   return dt->myclass;
}

void DecisionTreeSetClass(DecisionTreePtr dt, int theClass) {
   dt->myclass = theClass;
}

float DecisionTreeGetClassProb(DecisionTreePtr dt, int theClass) {
   return dt->classDistribution[theClass] / dt->distributionSampleCount;
}

void  DecisionTreeSetClassProb(DecisionTreePtr dt, int theClass, float prob) {
   if(dt->distributionSampleCount == 0) {
      dt->distributionSampleCount = 1;
   }

   dt->classDistribution[theClass] = prob * dt->distributionSampleCount;
}

void DecisionTreeAddToClassDistribution(DecisionTreePtr dt, ExamplePtr e) {
   dt->distributionSampleCount++;

   dt->classDistribution[ExampleGetClass(e)]++;
}

float DecisionTreeGetClassDistributionSampleCount(DecisionTreePtr dt) {
   return dt->distributionSampleCount;
}

void DecisionTreeZeroClassDistribution(DecisionTreePtr dt) {
   int i;

   for(i = 0 ; i < ExampleSpecGetNumClasses(dt->spec) ; i++) {
      dt->classDistribution[i] = 0;
   }
   dt->distributionSampleCount = 0;
}


void DecisionTreeSetTypeLeaf(DecisionTreePtr dt) {
   int i;

   if(dt->nodeType != dtnLeaf && dt->nodeType != dtnGrowing) {
      /* free children */
      for(i = VALLength(dt->children) - 1 ;  i >= 0 ; i--) {
         DecisionTreeFree(VALRemove(dt->children, i));
      }
      VALFree(dt->children);
      dt->children = 0;
   }

   dt->nodeType = dtnLeaf;
}

void DecisionTreeSetTypeGrowing(DecisionTreePtr dt) {
   dt->nodeType = dtnGrowing;
}

void DecisionTreeSplitOnDiscreteAttribute(DecisionTreePtr dt, int attNum) {
   int i;

   dt->nodeType = dtnDiscrete;
   dt->splitAttribute = attNum;

   dt->children = VALNew();

   for(i = 0 ; i < ExampleSpecGetAttributeValueCount(dt->spec, attNum); i++) {
      VALAppend(dt->children, DecisionTreeNew(dt->spec));
   }
}

void DecisionTreeSplitOnContinuousAttribute(DecisionTreePtr dt,
                                            int attNum, float threshold) {
   dt->nodeType = dtnContinuous;
   dt->splitAttribute = attNum;

   dt->splitThreshold = threshold;

   dt->children = VALNew();
   VALAppend(dt->children, DecisionTreeNew(dt->spec));
   VALAppend(dt->children, DecisionTreeNew(dt->spec));
}

int DecisionTreeGetChildCount(DecisionTreePtr dt) {
   if(dt->children) {
      return VALLength(dt->children);
   } else {
      return 0;
   }
}

DecisionTreePtr DecisionTreeGetChild(DecisionTreePtr dt, int index) {
   if(dt->children) {
      return VALIndex(dt->children, index);
   } else {
      return 0;
   }
}


DecisionTreePtr DecisionTreeOneStepClassify(DecisionTreePtr dt, ExamplePtr e) {
   if(dt->nodeType == dtnLeaf || dt->nodeType == dtnGrowing) {
      return dt;
   }
   if(dt->nodeType == dtnDiscrete) {
      if(ExampleIsAttributeUnknown(e, dt->splitAttribute)) {
	/* HERE do something smarter */
         return VALIndex(dt->children, 0);
      } else {
         return VALIndex(dt->children, 
               ExampleGetDiscreteAttributeValue(e, dt->splitAttribute));
      }
   } else if(dt->nodeType == dtnContinuous) {
      if(ExampleIsAttributeUnknown(e, dt->splitAttribute)) {
	/* HERE do something smarter */
         return VALIndex(dt->children, 0);
      } else if(ExampleGetContinuousAttributeValue(e, dt->splitAttribute) >=
		dt->splitThreshold) {
         return VALIndex(dt->children, 1);
      } else { /* the value is < the threshold */
         return VALIndex(dt->children, 0);
      }      
   }

   return dt;
}

int DecisionTreeClassify(DecisionTreePtr dt, ExamplePtr e) {
   int depth = 1;
   DecisionTreePtr current, last;

   last = dt;
   current = DecisionTreeOneStepClassify(last, e);

   while((depth < 50000) && (last != current)) {

      last = current;
      current = DecisionTreeOneStepClassify(last, e);
      depth++;
   }

   //printf("depth %d\n", depth);

   DebugWarn(last != current,
        "At depth 50000 in DecisionTreeClassify, something must be wrong.\n");

   return current->myclass;
}

void DecisionTreeGatherLeaves(DecisionTreePtr dt, VoidAListPtr list) {
   int i;

   if(dt->nodeType == dtnLeaf) {
      VALAppend(list, dt);
   } else if(dt->nodeType == dtnDiscrete || dt->nodeType == dtnContinuous) {
      for(i = 0 ; i < VALLength(dt->children) ; i++) {
         DecisionTreeGatherLeaves(VALIndex(dt->children, i), list);
      }
   }
}

void DecisionTreeGatherGrowingNodes(DecisionTreePtr dt, VoidAListPtr list) {
   int i;

   if(dt->nodeType == dtnGrowing) {
      VALAppend(list, dt);
   } else if(dt->nodeType == dtnDiscrete || dt->nodeType == dtnContinuous) {
      for(i = 0 ; i < VALLength(dt->children) ; i++) {
         DecisionTreeGatherGrowingNodes(VALIndex(dt->children, i), list);
      }
   }
}

int  DecisionTreeCountNodes(DecisionTreePtr dt) {
   int sum;
   int i;

   if(dt->nodeType == dtnGrowing || dt->nodeType == dtnLeaf) {
      sum = 1;
   } else if(dt->nodeType == dtnDiscrete || dt->nodeType == dtnContinuous) {
      sum = 1;
      for(i = 0 ; i < VALLength(dt->children) ; i++) {
         sum += DecisionTreeCountNodes(VALIndex(dt->children, i));
      }
   } else {
      /* how did we get here? */
      DebugWarn(1, "There is an odd condition in DecisionTreeCountNodes.\n");
      sum = 1;
   }

   return sum;
}

static void _GetMostCommonClass(DecisionTreePtr dt, long *counts) {
   int i;

   if(dt->nodeType == dtnGrowing) {
   } else if(dt->nodeType == dtnLeaf) {
      (counts[dt->myclass])++;
   } else if(dt->nodeType == dtnDiscrete || dt->nodeType == dtnContinuous) {
      for(i = 0 ; i < VALLength(dt->children) ; i++) {
         _GetMostCommonClass(VALIndex(dt->children, i), counts);
      }
   }
}

int DecisionTreeGetMostCommonClass(DecisionTreePtr dt) {
   long *counts = MNewPtr(sizeof(long) * ExampleSpecGetNumClasses(dt->spec));
   int i;
   int mostCommon;

   for(i = 0 ; i < ExampleSpecGetNumClasses(dt->spec) ; i++) {
      counts[i] = 0;
   }

   _GetMostCommonClass(dt, counts);

   mostCommon = 0;
   for(i = 1 ; i < ExampleSpecGetNumClasses(dt->spec) ; i++) {
      if(counts[i] > counts[mostCommon]) {
         mostCommon = i;
      }
   }

   MFreePtr(counts);

   return mostCommon;
}

void DecisionTreeSetGrowingData(DecisionTreePtr dt, void *data) {
   dt->growingData = data;
}

void *DecisionTreeGetGrowingData(DecisionTreePtr dt) {
   return dt->growingData;
}

static void _PrintSpaces(FILE *out, int num) {
   int i;

   for(i = 0 ; i < num ; i++) {
      fprintf(out, " ");
   }
}

static void _PrintHelp(DecisionTreePtr dt, FILE *out, int indent) {
   int i;

   _PrintSpaces(out, indent);
   if(dt->nodeType == dtnLeaf) {
     //fprintf(out, "(leaf: %d)\n", dt->myclass);
      fprintf(out, "(leaf: %s)\n", 
             ExampleSpecGetClassValueName(dt->spec, dt->myclass));
   } else if(dt->nodeType == dtnDiscrete) {
      fprintf(out, "(split on %s:\n",
             ExampleSpecGetAttributeName(dt->spec, dt->splitAttribute));
      for(i = 0 ; i < VALLength(dt->children) ; i++) {
         _PrintSpaces(out, indent + 1);
         fprintf(out, "%s\n",
          ExampleSpecGetAttributeValueName(dt->spec, dt->splitAttribute, i));
         _PrintHelp(VALIndex(dt->children, i), out, indent + 2);
      }
      _PrintSpaces(out, indent);
      fprintf(out, ")\n");
   } else if(dt->nodeType == dtnContinuous) {
      fprintf(out, "(split on %s:\n",
             ExampleSpecGetAttributeName(dt->spec, dt->splitAttribute));

      /* left child */
      _PrintSpaces(out, indent + 1);
      fprintf(out, "< %f\n", dt->splitThreshold);
      _PrintHelp(VALIndex(dt->children, 0), out, indent + 2);

      /* right child */
      _PrintSpaces(out, indent + 1);
      fprintf(out, ">= %f\n", dt->splitThreshold);
      _PrintHelp(VALIndex(dt->children, 1), out, indent + 2);


      _PrintSpaces(out, indent);
      fprintf(out, ")\n");
   } else if(dt->nodeType == dtnGrowing) {
      fprintf(out, "(growing)\n");
   }
}

void DecisionTreePrint(DecisionTreePtr dt, FILE *out) {
   _PrintHelp(dt, out, 0);
}

static void _PrintStatsHelper(DecisionTreePtr dt, long *leavesAtLevel, long *leaves, int level, int maxLevel) {
   int i;

   if(dt->nodeType == dtnGrowing || dt->nodeType == dtnLeaf) {
      if(level < maxLevel) {
         (leavesAtLevel[level])++;
      }
      (*leaves)++;
   } else if(dt->nodeType == dtnDiscrete || dt->nodeType == dtnContinuous) {
      for(i = 0 ; i < VALLength(dt->children) ; i++) {
         _PrintStatsHelper(VALIndex(dt->children, i), leavesAtLevel, leaves, level + 1, maxLevel);
      }
   }
}

void DecisionTreePrintStats(DecisionTreePtr dt, FILE *out) {
   long leavesAtLevel[50];
   long leaves;
   int i, maxWithLeaves;

   leaves = 0;
   for(i = 0 ; i < 50 ; i++) {
      leavesAtLevel[i] = 0;
   }

   _PrintStatsHelper(dt, leavesAtLevel, &leaves, 0, 50);

   maxWithLeaves = 0;
   for(i = 0 ; i < 50 ; i++) {
      if(leavesAtLevel[i] > 0) {
         maxWithLeaves = i;
      }
   }

   for(i = 0 ; i <= maxWithLeaves ; i++) {
      fprintf(out, "%ld ", leavesAtLevel[i]);
   }
   fprintf(out, "\n");
}

/* code mostly stolen from c45 */
static void _StreamIn(FILE *in, char *s, int n) {
   while(n--) {
      *s++ = getc(in);
   }
}

static void _StreamOut(FILE *out, char *s, int n) {
   while(n--) {
      putc(*s++, out);
   }
}


/* HERE this only reads in leaves, continuous splits & discrete splits.
    If the trees have other things it will probably crash */
static DecisionTreePtr _DecisionTreeReadC45(FILE *in, ExampleSpecPtr spec) {
   int i;
   DecisionTreePtr dt;
   short type, leaf, tested, forks;
   float items, errors, lower, upper, cut;

   dt = DecisionTreeNew(spec);
 
   _StreamIn(in, (char *)(&type), sizeof(short));
   _StreamIn(in, (char *)(&leaf), sizeof(short));
   _StreamIn(in, (char *)(&items), sizeof(float));
   _StreamIn(in, (char *)(&errors), sizeof(float));

   dt->myclass = leaf;


   //classDist = MNewPtr(sizeof(float) * ExampleSpecGetNumClasses(spec));
   //_StreamIn(in, (char *)classDist, 
   //                     sizeof(float) * ExampleSpecGetNumClasses(spec));
   _StreamIn(in, (char *)(dt->classDistribution), 
                        sizeof(float) * ExampleSpecGetNumClasses(spec));
   for(i = 0 ; i < ExampleSpecGetNumClasses(spec) ; i++) {
      dt->distributionSampleCount += dt->classDistribution[i];
   }
   //MFreePtr(classDist);

   if(type != 0) { /* if we don't have a leaf */
      _StreamIn(in, (char *)(&tested), sizeof(short));
      _StreamIn(in, (char *)(&forks), sizeof(short));
      dt->splitAttribute = tested;

      switch(type) {
       case 1: /* BrDiscr */
         dt->nodeType = dtnDiscrete;

         break;
       case 2: /* ThreshContin */
         dt->nodeType = dtnContinuous;

         _StreamIn(in, (char *)(&cut), sizeof(float));
         _StreamIn(in, (char *)(&upper), sizeof(float));
         _StreamIn(in, (char *)(&lower), sizeof(float));

         dt->splitThreshold = cut;

         break;
      }

      dt->children = VALNew();
      for(i = 0 ; i < forks ; i++) {
         VALAppend(dt->children, _DecisionTreeReadC45(in, spec));
      }

   } else {
      dt->nodeType = dtnLeaf;
   }
   
   return dt;
}

DecisionTreePtr DecisionTreeReadC45(FILE *in, ExampleSpecPtr spec) {
   getc(in); /* flush something */
   return _DecisionTreeReadC45(in, spec);
}

DecisionTreePtr DecisionTreeRead(FILE *in, ExampleSpecPtr spec) {
   int i, forks;
   DecisionTreePtr dt = DecisionTreeNew(spec);

   _StreamIn(in, (char *)(&(dt->nodeType)), sizeof(NodeType));

   switch(dt->nodeType) {
    case dtnLeaf:
    case dtnGrowing:
      _StreamIn(in, (char *)(&(dt->myclass)), sizeof(int));
      break;
    case dtnContinuous:
    case dtnDiscrete:
      _StreamIn(in, (char *)(&(dt->splitThreshold)), sizeof(float));
      _StreamIn(in, (char *)(&(dt->splitAttribute)), sizeof(int));
      _StreamIn(in, (char *)(&forks), sizeof(int));
      dt->children = VALNew();
      for(i = 0 ; i < forks ; i++) {
         VALAppend(dt->children, DecisionTreeRead(in, spec));
      }
      break;
   }
   return dt;
}

void DecisionTreeWrite(DecisionTreePtr dt, FILE *out) {
   int i;
   int forks;

   _StreamOut(out, (char *)(&(dt->nodeType)), sizeof(NodeType));
   switch(dt->nodeType) {
    case dtnLeaf:
    case dtnGrowing:
      _StreamOut(out, (char *)(&(dt->myclass)), sizeof(int));
      break;
    case dtnContinuous:
    case dtnDiscrete:
      _StreamOut(out, (char *)(&(dt->splitThreshold)), sizeof(float));
      _StreamOut(out, (char *)(&(dt->splitAttribute)), sizeof(int));
      forks = DecisionTreeGetChildCount(dt);
      _StreamOut(out, (char *)(&(forks)), sizeof(int));
      for(i = 0 ; i < forks ; i++) {
         DecisionTreeWrite(DecisionTreeGetChild(dt, i), out);
      }
      break;
   }
}
