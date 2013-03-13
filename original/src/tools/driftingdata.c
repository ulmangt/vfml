#include "vfml.h"
#include <stdio.h>
#include <string.h>
#include <sys/times.h>
#include <time.h>

char *gFileStem      = "DF";
int   gNumDiscAttributes = 100;
int   gNumContAttributes = 10;
int   gNumClasses    = 2;
unsigned long  gNumTrainExamples = 50000;
unsigned long  gNumTestExamples = 50000;
unsigned long  gNumPruneExamples = 0;
float gErrorLevel = 0.00;
long  gSeed = -1;
int   gMessageLevel = 0;
int   gStdout = 0;
int   gMutate = 10000;
FILE* gOutputFile;
int gDriftType = 0;

float gPrunePercent = 25;
int gFirstPruneLevel = 3;
int gMaxLevel = 18;

typedef struct _RebuildInfo_ {
  AttributeTrackerPtr at;
  int level;
} RebuildInfo, *RebuildInfoPtr;

/* Some hack globals */
ExampleSpecPtr gEs; /* will be set to the spec for the run */
ExampleGeneratorPtr gEg;
//DecisionTreePtr gDT;


static char *_AllocateString(char *first) {
   char *second = MNewPtr(sizeof(char) * (strlen(first) + 1));

   strcpy(second, first);

   return second;
}

static void _processArgs(int argc, char *argv[]) {
   int i;

   /* HERE on the ones that use the next arg make sure it is there */
   for(i = 1 ; i < argc ; i++) {
      if(!strcmp(argv[i], "-f")) {
         gFileStem = argv[i+1];
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-h")) {
         printf("Generate a synthetic dataset\n");
         printf("   -f <stem name> (default DF)\n");
         printf("   -discrete <number of discrete attributes> (default 100)\n");
         printf("   -continuous <number of continous attributes> (default 10)\n");
         printf("   -classes <number of classes> (default 2)\n");
         printf("   -train <size of training set> (default 50000)\n");
         printf("   -test  <size of testing set> (default 50000)\n");
         printf("   -prune  <size of prune set> (default 50000)\n");
         printf("   -stdout  output the trainset to stdout (default to <stem>.data)\n");
         printf("   -noise <percentage noise (as float (eg: 10.2 is 10.2%%)> (default 0)\n");

         printf("   -prunePercent <%%of nodes to prune at each level> (default is 25 that's 25%%)\n");
         printf("   -firstPruneLevel <don't prune nodes before this level> (default is 3)\n");
         printf("   -maxPruneLevel <prune every node after this level> (default is 18)\n");
         printf("   -seed <random seed> (default to random)\n");
	 printf("   -driftType <0 = none, 1 = change class>\n");
         printf("   -mutate <number of examples between decision tree mutations> (default 30,000)\n");
         printf("   -pruneSetSize <size of the prune set to create> (default 0)\n");
         printf("   -v increase the message level\n");
         exit(0);
      } else if(!strcmp(argv[i], "-discrete")) {
         sscanf(argv[i+1], "%d", &gNumDiscAttributes);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-continuous")) {
         sscanf(argv[i+1], "%d", &gNumContAttributes);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-classes")) {
         sscanf(argv[i+1], "%d", &gNumClasses);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-driftType")) {
         sscanf(argv[i+1], "%d", &gDriftType);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-train")) {
         sscanf(argv[i+1], "%lu", &gNumTrainExamples);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-test")) {
         sscanf(argv[i+1], "%lu", &gNumTestExamples);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-prune")) {
         sscanf(argv[i+1], "%lu", &gNumPruneExamples);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-mutate")) {
         sscanf(argv[i+1], "%lu", &gMutate);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-noise")) {
         sscanf(argv[i+1], "%f", &gErrorLevel);
         gErrorLevel /= 100.0;
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-prunePercent")) {
         sscanf(argv[i+1], "%f", &gPrunePercent);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-seed")) {
         sscanf(argv[i+1], "%lu", &gSeed);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-firstPruneLevel")) {
         sscanf(argv[i+1], "%d", &gFirstPruneLevel);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-maxPruneLevel")) {
         sscanf(argv[i+1], "%d", &gMaxLevel);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-v")) {
         gMessageLevel++;
      } else if(!strcmp(argv[i], "-stdout")) {
         gStdout = 1;
      } else {
         printf("unknown argument '%s', use -h for help\n", argv[i]);
         exit(0);
      }
   }
}

static RebuildInfoPtr _RebuildInfoNew(AttributeTrackerPtr at, int level) {
  RebuildInfoPtr ri = MNewPtr(sizeof(RebuildInfo));

  ri->at = at;
  ri->level = level;

  return ri;
}

static void _RebuildInfoFree(RebuildInfoPtr ri) {
  AttributeTrackerFree(ri->at);
  MFreePtr(ri);
}

static int _PickSplitAttribute(AttributeTrackerPtr at)
{
  int splitAttribute, found, i;
  int activeCount = AttributeTrackerNumActive(at);

  splitAttribute = RandomRange(0, activeCount - 1);
  found = 0;
  for(i = 0; i < ExampleSpecGetNumAttributes(gEs) && !found; i++) {
    if(AttributeTrackerIsActive(at, i)) {
      if(splitAttribute == 0) {
	splitAttribute = i;
	found = 1;
      } else {
	splitAttribute--;
      }
    }
  }
  return splitAttribute;
}

double *_attMin;
double *_attMax;

static void _CreateConceptTreeHelper(DecisionTreePtr dt, long seed, long node, int level) {
   int prunePercent;
   int i, activeCount, found, splitAttribute;
   double thresh, min, max;
   RebuildInfoPtr ri = (RebuildInfoPtr)DecisionTreeGetGrowingData(dt);
   RandomSeed(seed * node);

   /* test for prune */
   prunePercent = gPrunePercent;
   if(level < gFirstPruneLevel) {
      prunePercent = 0;
   }

   if(level > gMaxLevel || RandomRange(0, 100) < prunePercent
                        || AttributeTrackerAreAllInactive(ri->at)) {
      DecisionTreeSetTypeLeaf(dt);
      DecisionTreeSetClass(dt, RandomRange(0, gNumClasses - 1));
      return;
   }

   splitAttribute = _PickSplitAttribute(ri->at);

   if(ExampleSpecIsAttributeDiscrete(gEs, splitAttribute)) {
     int i;
     DecisionTreePtr curChild;
     AttributeTrackerMarkInactive(ri->at, splitAttribute);
     
     DecisionTreeSplitOnDiscreteAttribute(dt, splitAttribute);
     for(i = 0; i < DecisionTreeGetChildCount(dt); i++ ) {
       AttributeTrackerPtr curAt = AttributeTrackerClone(ri->at);
       RebuildInfoPtr curRi = _RebuildInfoNew(curAt, level + 1);
       curChild = DecisionTreeGetChild( dt, i );
       DecisionTreeSetGrowingData( curChild, curRi );
     }
     _CreateConceptTreeHelper(DecisionTreeGetChild(dt, 0), 
			      seed, node * 2, level + 1);
     _CreateConceptTreeHelper(DecisionTreeGetChild(dt, 1), 
			      seed, (node * 2) + 1, level + 1);
     AttributeTrackerMarkActive(ri->at, splitAttribute);
   } else {
     // NOTE: attribute tracker not created for continuous attribute
      /* must be continuous */
      thresh = (double)RandomRange(_attMin[splitAttribute] * 10000, 
				   _attMax[splitAttribute] * 10000) / (double)10000;
      DecisionTreeSplitOnContinuousAttribute(dt, splitAttribute, thresh);

      max = _attMax[splitAttribute];
      _attMax[splitAttribute] = thresh;
      if(_attMax[splitAttribute] - _attMin[splitAttribute] < 0.05) {
         AttributeTrackerMarkInactive(ri->at, splitAttribute);
      }
      _CreateConceptTreeHelper(DecisionTreeGetChild(dt, 0), 
			       seed, node * 2, level + 1);
      AttributeTrackerMarkActive(ri->at, splitAttribute);

      _attMax[splitAttribute] = max;

      min = _attMin[splitAttribute];
      _attMin[splitAttribute] = thresh;

      if(_attMax[splitAttribute] - _attMin[splitAttribute] < 0.05) {
         AttributeTrackerMarkInactive(ri->at, splitAttribute);
      }
      _CreateConceptTreeHelper(DecisionTreeGetChild(dt, 1), 
			       seed, (node * 2) + 1, level + 1);
      AttributeTrackerMarkActive(ri->at, splitAttribute);
      
      _attMin[splitAttribute] = min;
   }
}

static int _DTIsPure(DecisionTreePtr dt, int *class) {
   int i;
   int theClass, tmpClass, pure;

   if(DecisionTreeIsLeaf(dt) || DecisionTreeIsNodeGrowing(dt)) {
      *class = DecisionTreeGetClass(dt);
      return 1;
   } else {
      tmpClass = -1;
      pure = _DTIsPure(DecisionTreeGetChild(dt, 0), &theClass);
      for(i = 1; i < DecisionTreeGetChildCount(dt) && pure ; i++) {
         pure = _DTIsPure(DecisionTreeGetChild(dt, i), &tmpClass);
         if(tmpClass != theClass) {
            pure = 0;
         }
      }
   }
   *class = theClass;
   return pure;
}


int _found = 0;
static void _PrunePureSubtrees(DecisionTreePtr dt) {
   int i;
   int class;

   if(!DecisionTreeIsLeaf(dt)) {
      if(_DTIsPure(dt, &class)) {
         //printf("found: %d class: %d nodes: %d\n", ++_found, class,
         //                              DecisionTreeCountNodes(dt));
         //DecisionTreePrint(dt, stdout);
         DecisionTreeSetClass(dt, class);
         DecisionTreeSetTypeLeaf(dt);
         //printf("\n");
         //DecisionTreePrint(dt, stdout);
         //printf("---\n"); 
         //fflush(stdout);
      } else {
         for(i = 0; i < DecisionTreeGetChildCount(dt) ; i++) {
            _PrunePureSubtrees(DecisionTreeGetChild(dt, i));
         }

      }
   }
}

static DecisionTreePtr _CreateConceptTree(long seed) {
   void *oldState;
   int i;
   int maxLeafLevel;
   AttributeTrackerPtr at = AttributeTrackerInitial(gEs);
   DecisionTreePtr dt;
   RebuildInfoPtr ri;

   _attMin = MNewPtr(sizeof(double) * ExampleSpecGetNumAttributes(gEs));
   _attMax = MNewPtr(sizeof(double) * ExampleSpecGetNumAttributes(gEs));
   for(i = 0 ; i < ExampleSpecGetNumAttributes(gEs) ; i++) {
      _attMin[i] = 0;
      _attMax[i] = 1;
   }

   oldState = (void *)RandomNewState(10);

   dt = DecisionTreeNew(gEs);
   ri = _RebuildInfoNew(at, 0);
   DecisionTreeSetGrowingData(dt, ri);
   _CreateConceptTreeHelper(dt, seed, 1, 0);

   oldState = (void *)RandomSetState(oldState);
   RandomFreeState(oldState);

   _PrunePureSubtrees(dt);

   return dt;
}

static ExampleSpecPtr _CreateExampleSpec(void) {
   ExampleSpecPtr es = ExampleSpecNew();
   AttributeSpecPtr as;
   char tmpStr[255];
   int i, j, valueCount, num;

   for(i = 0 ; i < gNumClasses ; i++) {
      sprintf(tmpStr, "class %d", i);
      ExampleSpecAddClass(es, _AllocateString(tmpStr));
   }

   for(i = 0 ; i < gNumDiscAttributes ; i++) {
      sprintf(tmpStr, "d-attribute %d", i);
      num = ExampleSpecAddDiscreteAttribute(es, _AllocateString(tmpStr));

      valueCount = 2;
      for(j = 0 ; j < valueCount ; j++) {
         sprintf(tmpStr, "%d-%d", i, j);
         ExampleSpecAddAttributeValue(es, num, _AllocateString(tmpStr));
      }
   }

   for(i = 0 ; i < gNumContAttributes ; i++) {
      sprintf(tmpStr, "c-attribute %d", i);
      num = ExampleSpecAddContinuousAttribute(es, _AllocateString(tmpStr));
   }

   return es;
}

static void _reclassifyTestExamples(DecisionTreePtr root) {
  char oldFileName[255];
  char newFileName[255];
  FILE *oldFile;
  FILE *newFile;
  ExamplePtr e;
  int class, res;
  long i;
  
  sprintf(oldFileName, "%s.test", gFileStem);
  oldFile = fopen(oldFileName, "r");
  DebugError(oldFile == 0, "Cannot open test file for reclassification");

  sprintf(newFileName, "%s.tmp", gFileStem);
  newFile = fopen(newFileName, "w");

  for(i = 0; i < gNumTestExamples; i++) {
    int oldClass;
    e = ExampleRead(oldFile, root->spec);
    DebugError(e == 0, "Unable to read example from test file");
    oldClass = ExampleGetClass( e );
    class = DecisionTreeClassify(root, e);
    ExampleSetClass(e, class);
    ExampleWrite(e, newFile);
    ExampleFree(e);
  }
  fclose(oldFile);
  fclose(newFile);
  res = remove(oldFileName);
  DebugError(res != 0, "Unable to remove old test file after reclassification");
  res = rename(newFileName, oldFileName);
  DebugError(res != 0, "Unable to rename new test file after reclassification");
}

static void _gatherNodesHelper(DecisionTreePtr node, VoidAListPtr nodes) {
  int numChildren; 
  int i;
  DecisionTreePtr curChild;
  
  VALAppend( nodes, node );
  if (!DecisionTreeIsLeaf(node)) {
    numChildren = DecisionTreeGetChildCount( node );
    for (i = 0; i < numChildren; i++) {
      curChild = DecisionTreeGetChild( node, i );
      _gatherNodesHelper( curChild, nodes );
    }
  }
}

static VoidAListPtr _gatherNodes(DecisionTreePtr root){
  VoidAListPtr nodes = VALNew();
  _gatherNodesHelper(root, nodes);
  return nodes;
}

static void _changeClass( DecisionTreePtr root ) {
  VoidAListPtr nodes = VALNew();
  int nodeIndex, newClass;
  DecisionTreePtr node;
  int numClasses = ExampleSpecGetNumClasses(root->spec);

  DecisionTreeGatherLeaves(root, nodes);
  fprintf( gOutputFile, "Gathered %d nodes\n", VALLength(nodes));
  fflush( gOutputFile );
  if (VALLength(nodes) == 0)
    return;
  nodeIndex = RandomRange(1, VALLength(nodes));  
  fprintf( gOutputFile, "Selected node %d\n", nodeIndex - 1 );
  fflush( gOutputFile );
  node = VALIndex(nodes, nodeIndex - 1);
  newClass = RandomRange(0, numClasses - 1);
  DecisionTreeSetClass(node, newClass);
}

static void _DriftingDTFree(DecisionTreePtr node)
{
  int numChildren;
  RebuildInfoPtr curRi;
  if (!DecisionTreeIsLeaf(node)) {
    int i;
    numChildren = DecisionTreeGetChildCount(node);
    for(i = 0; i < numChildren; i++) {
      DecisionTreePtr curChild = DecisionTreeGetChild(node, i);
      _DriftingDTFree(curChild);
    }
  }
  curRi = (RebuildInfoPtr)DecisionTreeGetGrowingData(node);
  _RebuildInfoFree(curRi);
}

static void _splitOnAttr(DecisionTreePtr root)
{
  VoidAListPtr nodes = VALNew();
  int nodeIndex;
  DecisionTreePtr node;
  int numChildren;
  RebuildInfoPtr ri;
  int splitAttribute;
  int i;

  nodes = _gatherNodes( root );
  do {
    nodeIndex = RandomRange(1, VALLength(nodes));
    node = VALIndex(nodes, nodeIndex);
    ri = (RebuildInfoPtr)DecisionTreeGetGrowingData(node);
  } while ((ri->level >= gMaxLevel) || (ri->level < gFirstPruneLevel));

  if (AttributeTrackerAreAllInactive(ri->at)) {
    fprintf(gOutputFile, "Warning: all attributes are inactive\n");
    fflush(gOutputFile);
    return;
  }
  
  // Delete children
  if (!DecisionTreeIsLeaf(node)) {
    numChildren = DecisionTreeGetChildCount(node);
    AttributeTrackerMarkActive(ri->at, node->splitAttribute);
    for(i = 0; i < numChildren; i++) {
      DecisionTreePtr curChild = DecisionTreeGetChild(node, i);
      _DriftingDTFree(curChild);
      DecisionTreeFree(curChild);
    }
  }
    
  splitAttribute = _PickSplitAttribute(ri->at);    
  DecisionTreeSplitOnDiscreteAttribute(node, splitAttribute);
  for(i = 0; i < DecisionTreeGetChildCount(node); i++) {
    AttributeTrackerPtr curAt = AttributeTrackerClone(ri->at);
    RebuildInfoPtr curRi;
    DecisionTreePtr curChild;
    AttributeTrackerMarkInactive(curAt, splitAttribute);
    curRi = _RebuildInfoNew(curAt, ri->level + 1);
    curChild = DecisionTreeGetChild(node, i);
    DecisionTreeSetGrowingData(curChild, curRi);
    DecisionTreeSetTypeLeaf(curChild);
    DecisionTreeSetClass(node, RandomRange(0, gNumClasses - 1));
  }
}

static void _mutate(DecisionTreePtr root)
{
  int rand;
  if ( gDriftType == 0 )
    return;
  if ( gDriftType == 1 )
    _changeClass( root );
  else if ( gDriftType == 2 ) {
    rand = RandomRange(1, 100);
    if (rand < 67)
      _changeClass( root );
    else
      _splitOnAttr( root );
  }
  // int randType = RandomRange(1, 100);
  // if ( randType < 70 )
  /*  else if ( randType < 90 )
    _splitOnAttr( node );
  else
    _deleteChildren( node );
  */
  _reclassifyTestExamples(root);
}

static void _makeData(DecisionTreePtr dt, long seed) {
   char fileNames[255];
   FILE *fileOut;
   ExamplePtr e;
   int class;
   long i, j, initialAlloc, allocSize;
   ExampleGeneratorPtr eg;
   VoidAListPtr examples;

   eg = ExampleGeneratorNew(gEs, seed);

   if(!gStdout) {
      sprintf(fileNames, "%s.data", gFileStem);
      fileOut = fopen(fileNames, "w");
   } else {
      fileOut = stdout;
   }

   for(i = 0 ; i < gNumTrainExamples ; i++) {
     if((i != 0) && (i%gMutate == 0))
       _mutate( dt );
     e = ExampleGeneratorGenerate(eg);
     class = DecisionTreeClassify(dt, e);

     ExampleSetClass(e, class);
     ExampleAddNoise(e, gErrorLevel, 1, 0);
     ExampleWrite(e, fileOut);
     ExampleFree(e);
   }

   if(!gStdout) {
      fclose(fileOut);
   }

   ExampleGeneratorFree(eg);
}

static void _writeExampleSpec() {
  char fileNames[255];
  FILE *fileOut;  

  sprintf(fileNames, "%s.names", gFileStem);
  fileOut = fopen(fileNames, "w");
  ExampleSpecWrite(gEs, fileOut);
  fclose(fileOut);
}

static void _writeConceptTree(DecisionTreePtr dt)
{
  char fileNames[255];
  FILE *fileOut;

  sprintf(fileNames, "%s.concept", gFileStem);
  fileOut = fopen(fileNames, "w");
  DecisionTreePrint(dt, fileOut);
  fclose(fileOut);
}

static void _writeStats(DecisionTreePtr dt, long seed)
{
  char fileNames[255];
  FILE *fileOut;

  sprintf(fileNames, "%s.stats", gFileStem);
  fileOut = fopen(fileNames, "w");
  
  fprintf(fileOut, "Concept tree has %d nodes.\n Leaves by level: ", DecisionTreeCountNodes(dt));   
  DecisionTreePrintStats(dt, fileOut);
  
  fprintf(fileOut, "Num Attributes: %d discrete & %d continuous\n", gNumDiscAttributes, gNumContAttributes);
  fprintf(fileOut, "Num Classes %d\n", gNumClasses);
  
  fprintf(fileOut, "Num train examples %ld\n", gNumTrainExamples);
  fprintf(fileOut, "Num test examples %ld\n", gNumTestExamples);
  fprintf(fileOut, "Num prune examples %ld\n", gNumPruneExamples);
  fprintf(fileOut, "Noise level %.2f%%\n", gErrorLevel * 100);
  fprintf(fileOut, "Prune percent %.0f%%\n", gPrunePercent);
  fprintf(fileOut, "First prune level %d\n", gFirstPruneLevel);
  fprintf(fileOut, "Max level %d\n", gMaxLevel);
  fprintf(fileOut, "Example seed %d\n", seed);
  
  fclose(fileOut);
}

static void _writeTestExamples(DecisionTreePtr dt, ExampleGeneratorPtr eg) {
  char fileNames[255];
  FILE *fileOut;
  ExamplePtr e;
  int class;
  long i;
  
  sprintf(fileNames, "%s.test", gFileStem);
  fileOut = fopen(fileNames, "w");
  
  for(i = 0 ; i < gNumTestExamples ; i++) {
    e = ExampleGeneratorGenerate(eg);
    class = DecisionTreeClassify(dt, e);
    
    ExampleSetClass(e, class);
    ExampleAddNoise(e, gErrorLevel, 1, 0);
    ExampleWrite(e, fileOut);
    ExampleFree(e);
  }
  fclose(fileOut);  
}

static void _writePruneExamples(DecisionTreePtr dt, ExampleGeneratorPtr eg) {
  char fileNames[255];
  FILE *fileOut;
  ExamplePtr e;
  int class;
  long i;

  if(gNumPruneExamples > 0) {
    sprintf(fileNames, "%s.prune", gFileStem);
    fileOut = fopen(fileNames, "w");
    
    for(i = 0 ; i < gNumPruneExamples ; i++) {
      e = ExampleGeneratorGenerate(eg);
      class = DecisionTreeClassify(dt, e);
      
      ExampleSetClass(e, class);
      ExampleAddNoise(e, gErrorLevel, 1, 0);
      ExampleWrite(e, fileOut);
      ExampleFree(e);
    }
    fclose(fileOut);
    
  }
}

static void _initializeTree( DecisionTreePtr dt, long seed ) {
  ExampleGeneratorPtr eg;

  _writeConceptTree(dt);
  _writeStats( dt, seed );

  eg = ExampleGeneratorNew(gEs, RandomRange(1, 10000));
  _writeTestExamples( dt, eg );
  _writePruneExamples( dt, eg );
  ExampleGeneratorFree(eg);
}

int main(int argc, char *argv[]) {
  int i;
  DecisionTreePtr dt;
  char outName[255];

  _processArgs(argc, argv);
  sprintf(outName, "%s.drift", gFileStem);
  gOutputFile = fopen(outName, "w");
  DebugError( gOutputFile == 0, "Couldn't open drift output file");

  RandomInit();
  
  gEs = _CreateExampleSpec();
  _writeExampleSpec();

  if(gSeed != -1) {
    RandomSeed(gSeed);
  } else {
    gSeed = RandomRange(1, 30000);
    RandomSeed(gSeed);
  }
  dt = _CreateConceptTree(gSeed);
  _initializeTree( dt, gSeed );
  _makeData(dt, gSeed);
  DecisionTreeFree( dt );

  return 0;
}












