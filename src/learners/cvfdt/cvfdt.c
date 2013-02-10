#include "vfml.h"
#include "vfdt-engine.h"
#include "cvfdt.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <sys/times.h>
#include <time.h>

#define lg(x)       (log(x) / log(2))

char *gFileStem = "DF";
char *gSourceDirectory = ".";
int   gDoTests = 0;
int   gMessageLevel = 0;

int   gStdin           = 0;
float gSplitConfidence = (float)0.01;
float gTieConfidence   = (float)0.05;
int   gUseGini         = 0;
int   gRescans         = 1;
int   gChunk           = 300;
int   gGrowMegs        = 1000;
int	gWindowSize      = 50000;
int	gCacheSize	 = 10000;
int     gCurCacheIndex   = 0;
int     gLastFilledCacheIndex = -1;
int     gCurCachePos      = 0;
int	gCacheTestSet	  = 1;
int	gUseSchedule	  = 1;
int	gScheduleCount	  = 10000;
float	gScheduleMult	  = 1.44;
int   gIncrementalReporting = 0;
VoidAListPtr gCacheFiles;
FILE*   gTestOut  = 0;
unsigned long gNodeId  = 0;
VoidAListPtr gCache;
int gAltTestNum = 10000;
int gNumSwitches = 0;
int gNumPrunes = 0;
int gNumNewAlts = 0;
int gSplitLevels[100];
float gCumulSplitStrength;
int gCheckSize = 10000;

static void _printUsage(char  *name) {
  printf("%s : 'Concept-adapting Very Fast Decision Tree' induction\n", name);
  
  printf("-f <filestem>\tSet the name of the dataset (default DF)\n");
  printf("-source <dir>\tSet the source data directory (default '.')\n");
  printf("-u\t\tTest the learner's accuracy on data in <stem>.test\n");

  printf("-sc <allowed chance of error in each decision> (default 0.01 that's 1 percent)\n");
  printf("-stdin \t\t Reads training examples from stdin instead of from\n\t\t <stem>.data (default off) - NOTE this disables the rescans switch\n");
  printf("-tc <tie error> call a tie when hoeffding e < this. (default 0.05)\n");
  printf("-rescans <count> Naievely consider each example 'count' times\n\t\t with no concern for using it once per level of the induced\n\t\t tree (default 1)\n");
  printf("-chunk <count> wait until 'count' examples accumulate at a leaf\n\t\t before testing for a split (default 300)\n");
  printf("-growMegs <count> limit dynamic memory allocation to 'count'\n\t\t megabytes (default 1000)\n");
  printf("-window <count> number of examples used for context switching (default 50000)\n");
  printf("-cache <count> number of examples from the window to keep in\n\t\t memory (default 10000)\n");
  printf("-schedule <#> Run tests every # examples (default 10000).\n");
  printf("-altTest <#> Interval for building and testing alternate\n\t\t trees (default 10000).\n" );
  printf("-incrementalReporting \t As each example arrives test the\n\t\t learned model with it and then learn on it (default off).\n");
  printf("-v\t\tCan be used multiple times to increase the debugging output\n");
  printf("-checkSize <count> wait until 'count' examples accumulate\n\t\t before checking for questionable nodes(default 10000)\n");
}


static void _processArgs(int argc, char *argv[]) {
   int i;

   /* HERE on the ones that use the next arg make sure it is there */
   for(i = 1 ; i < argc ; i++) {
      if(!strcmp(argv[i], "-f")) {
         gFileStem = argv[i+1];
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-source")) {
         gSourceDirectory = argv[i+1];
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-u")) {
         gDoTests = 1;
      } else if(!strcmp(argv[i], "-v")) {
         gMessageLevel++;
         DebugSetMessageLevel(gMessageLevel);
      } else if(!strcmp(argv[i], "-h")) {
         _printUsage(argv[0]);
         exit(0);
      } else if(!strcmp(argv[i], "-stdin")) {
         gStdin = 1;
      } else if(!strcmp(argv[i], "-sc")) {
         sscanf(argv[i+1], "%f", &gSplitConfidence);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-tc")) {
         sscanf(argv[i+1], "%f", &gTieConfidence);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-gini")) {
         gUseGini = 1;
      } else if(!strcmp(argv[i], "-rescans")) {
         sscanf(argv[i+1], "%d", &gRescans);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-chunk")) {
         sscanf(argv[i+1], "%d", &gChunk);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-checkSize")) {
         sscanf(argv[i+1], "%d", &gCheckSize);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-growMegs")) {
         sscanf(argv[i+1], "%d", &gGrowMegs);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-window")) {
         sscanf(argv[i+1], "%d", &gWindowSize);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-cache")) {
         sscanf(argv[i+1], "%d", &gCacheSize);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-altTest")) {
         sscanf(argv[i+1], "%d", &gAltTestNum);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-incrementalReporting")) {
         gIncrementalReporting = 1;
      } else if(!strcmp(argv[i], "-schedule")) {
         gUseSchedule = 1;
         sscanf(argv[i+1], "%f", &gScheduleMult);
         /* ignore the next argument */
         i++;
      } else {
         printf("Unknown argument: %s.  use -h for help\n", argv[i]);
         exit(0);
      }
   }

   if(gMessageLevel >= 1) {
      printf("Stem: %s\n", gFileStem);
      printf("Source: %s\n", gSourceDirectory);

      printf("Split Confidence: %f\n", gSplitConfidence);
      printf("Tie Confidence: %f\n", gTieConfidence);

      if(gUseGini) {
         printf("Using Gini split index\n");
      } else {
         printf("Using information gain split index\n");
      }

      printf("Considering each example %d times.\n", gRescans);
      printf("Checking for splits for every %d examples at a leaf\n", gChunk);

      if(gDoTests) {
         printf("Running tests\n");
      }

      printf("Window size: %d\n", gWindowSize );
      printf("Message level: %d\n", gMessageLevel );
   }
}

/* Initialize the test cache */
void _initCache( ExampleSpecPtr es, VoidAListPtr testSet )
{
   FILE *exampleIn;
   char fileNames[255];
   ExamplePtr e;

   sprintf(fileNames, "%s/%s.test", gSourceDirectory, gFileStem);
   exampleIn = fopen(fileNames, "r");
   DebugError(exampleIn == 0, "Unable to open the .test file");

   e = ExampleRead(exampleIn, es);
   while(e != 0) {
     VALAppend(testSet, e);
     e = ExampleRead(exampleIn, es);
   }
   fclose(exampleIn);
}

long _incrementalErrors = 0;
long _incrementalTests  = 0;
static void _doIncrementalTest(VFDTPtr vfdt, ExampleSpecPtr es, ExamplePtr e) {
   DecisionTreePtr dt;

   dt = VFDTGetLearnedTree(vfdt);
   if(ExampleGetClass(e) != DecisionTreeClassify(dt, e)) {
      _incrementalErrors++;
   }
   _incrementalTests++;

   DecisionTreeFree(dt);
}

static void _doIncrementalReport(void) {
   printf("\tTested %ld examples, made %ld mistakes that's %.4lf%%\n", 
        _incrementalTests, _incrementalErrors, 
        (float)_incrementalErrors / (float)_incrementalTests);
}

/* Calculate accuracy of current tree against test set */
VoidAListPtr _testSet;
int          _testCacheInited = 0;
static void _doTests(ExampleSpecPtr es, DecisionTreePtr dt, long growingNodes,
		     long learnCount, long learnTime, long cacheTime, 
		     long allocation, int finalOutput,
		     int numQExamples, int numSwitches, int numPrunes ) {

   int oldPool = MGetActivePool();
   ExamplePtr e;
   long tested, errors;
   FILE *exampleIn;
   char fileNames[255];
   
   errors = tested = 0;
   
   /* don't track this allocation against other VFDT stuff */
   MSetActivePool(0);

   // open test file
   sprintf(fileNames, "%s/%s.test", gSourceDirectory, gFileStem);
   exampleIn = fopen(fileNames, "r");
   DebugError(exampleIn == 0, "Unable to open the .test file");

   // compare class of example in test set to class predicted
   // by current tree
   e = ExampleRead(exampleIn, es);
   while(e != 0) {
     if (!ExampleIsClassUnknown(e)) {
       tested++;
       if(ExampleGetClass(e) != DecisionTreeClassify(dt, e)) {
	 errors++;
       }
     }
     ExampleFree(e);
     e = ExampleRead(exampleIn, es);
   }
   fclose(exampleIn);

   /* if(!_testCacheInited) {
     _testSet = VALNew();
     _initCache( es, _testSet );
     _testCacheInited = 1;
   }
   
   for(i = 0 ; i < VALLength(_testSet) ; i++) {
     e = VALIndex(_testSet, i);
     if(!ExampleIsClassUnknown(e)) {
       tested++;
       if(ExampleGetClass(e) != DecisionTreeClassify(dt, e)) {
	 errors++;
       }			
     }
     }*/
   
   // Report accuracy
   if(finalOutput) {
     if(gMessageLevel >= 1) {
       printf("Tested %ld examples made %ld errors\n", (long)tested, (long)errors);
     }
     printf("%.4f\t%ld\n", ((float)errors/(float)tested) * 100, (long)DecisionTreeCountNodes(dt));
     fflush(stdout);
     fprintf(gTestOut,"%.4f\t%ld\n", ((float)errors/(float)tested) * 100, (long)DecisionTreeCountNodes(dt));
     fclose(gTestOut);
   } else {
     int i;
     int sumLevels = 0;
     float sumStrength = 0.0;
     float averageLevel = 0.0;
     float averageStrength = 0;
     for ( i = 0; i < gNumSwitches; i++ ) {
       sumLevels += gSplitLevels[i];
       sumStrength += pow(2.0, (float)-gSplitLevels[i]);
     }
     if ( gNumSwitches != 0 ) {
       averageLevel = (float)sumLevels/(float)gNumSwitches;
       averageStrength = pow(2.0, -averageLevel);
     }
     printf("learned from, error, nodes, growing, learn time, cache time, memory, switches, prunes, new alts\n");
     printf(">> %ld\t%.4f\t%ld\t%ld\t%.2lf\t%.2lf\t%.2f\t%d\t%d\t%d\n",
	    learnCount,
	    ((float)errors/(float)tested) * 100,
	    (long)DecisionTreeCountNodes(dt),
	    growingNodes,
	    ((double)learnTime) / 100,
	    ((double)cacheTime) / 100,
	    ((double)allocation / (float)(1024 * 1024)),
//	    ((float)numQExamples)/(float)tested,
	    gNumSwitches, gNumPrunes, gNumNewAlts);//,
	    //sumStrength, averageStrength);
     fprintf(gTestOut,">> %ld\t%.4f\t%ld\t%ld\t%.2lf\t%.2lf\t%.2f\t%d\t%d\t%d\n",
	     learnCount,
	     ((float)errors/(float)tested) * 100,
	     (long)DecisionTreeCountNodes(dt),
	     growingNodes,
	     ((double)learnTime) / 100,
	     ((double)cacheTime) / 100,
	     ((double)allocation / (float)(1024 * 1024)),
//	     ((float)numQExamples/(float)tested),
	     gNumSwitches, gNumPrunes, gNumNewAlts); //,
	     //sumStrength, averageStrength);
     fflush( gTestOut );
   }
   fflush(stdout); 
   
   MSetActivePool(oldPool);
}

/* Initialize names of cache files and put names in gCacheFiles */
void _initWindowFiles()
{
  int numFiles;
  char* fileName;
  int i;
  FILE* curFile;

  gCacheFiles = VALNew();
  numFiles = ceil( (float)gWindowSize / (float)gCacheSize );
  for ( i = 0; i < numFiles; i++ ) {
    fileName = MNewPtr(sizeof(char) * 10); 
    sprintf( fileName, "tmp%d.dat", i );
    VALAppend( gCacheFiles, fileName );
    curFile = fopen( fileName, "w" );
    DebugError( curFile == 0, "Unable to create tmp file" );
    fclose( curFile );
  }
}

int main(int argc, char *argv[]) {
   char fileNames[255];

   FILE *exampleIn;
   ExampleSpecPtr es;
   CVFDTPtr cvfdt;
   DecisionTreePtr dt;

   int iteration;

   _processArgs(argc, argv);

   gTestOut = fopen( "outc.txt", "w");
   DebugError( gTestOut == 0, "Unable to open test results file" );

   sprintf(fileNames, "%s/%s.names", gSourceDirectory, gFileStem);
   es = ExampleSpecRead(fileNames);
   DebugError(es == 0, "Unable to open the .names file (main)");

   /* initialize cvfdt */
   cvfdt = CVFDTCreateRoot(es, gSplitConfidence, gTieConfidence);
   VFDTSetUseGini(cvfdt->vfdt, gUseGini);
   VFDTSetProcessChunkSize(cvfdt->vfdt, gChunk);
   VFDTSetMaxAllocationMegs(cvfdt->vfdt, gGrowMegs);

   if(gMessageLevel >= 1) {
      printf("allocation %ld\n", MGetTotalAllocation());
   }

   _initWindowFiles();

   //wait(100000);
   
   sprintf(fileNames, "%s/%s.data", gSourceDirectory, gFileStem);

   for(iteration = 0 ; iteration < gRescans ; iteration++) {
     if ( gStdin ) {
       exampleIn = stdin;
     } else {
       exampleIn = fopen(fileNames, "r");
       DebugError(exampleIn == 0, "Unable to open the data file (main)");
     }
     CVFDTProcessExamples(cvfdt, exampleIn);
     if ( !gStdin )
       fclose(exampleIn);
   }

   if(gMessageLevel >= 1) {
      printf("done learning...\n");
      printf("   allocation %ld\n", MGetTotalAllocation());
   }

   if(gDoTests) {
     dt = CVFDTGetLearnedTree( cvfdt );
     _doTests( es, dt, cvfdt->vfdt->numGrowing, 0, 0, 0, 
	       MGetTotalAllocation(), 1, 0, 0, 0 );
     DecisionTreeFree( dt );
   }

   if(gMessageLevel >= 1) {
      printf("allocation %ld\n", MGetTotalAllocation());
      //CVFDTPrint( cvfdt, stdout );
      //DecisionTreePrint( cvfdt->dtreeNode, stdout );
   }

   return 0;
}

static void _DoMakeLeaf(VFDTPtr vfdt, DecisionTreePtr currentNode) {
   ExampleGroupStatsPtr egs = ((VFDTGrowingDataPtr)DecisionTreeGetGrowingData(currentNode))->egs;
   int mostCommonClass = ExampleGroupStatsGetMostCommonClass(egs);

   vfdt->numGrowing--;
   DecisionTreeSetClass(currentNode, mostCommonClass);

   //ExampleGroupStatsFree(egs);
   //MFreePtr(DecisionTreeGetGrowingData(currentNode));
}

/* NOTE: Leafify not working for some reason
static float _CalculateLeafifyIndex(DecisionTreePtr node, long totalSeen) {
   float percent;
   VFDTGrowingDataPtr gd = (VFDTGrowingDataPtr)DecisionTreeGetGrowingData(node);

   percent = (float)(gd->seenExamplesCount / totalSeen);

   if(ExampleGroupStatsNumExamplesSeen(gd->egs) > 0) {
      return percent * 
         (1 - (ExampleGroupStatsGetMostCommonClassCount(gd->egs) /
                (float)ExampleGroupStatsNumExamplesSeen(gd->egs)));
   } else {
      return percent;
   }
}

static void _LeafifyLeastImportantNodes(VFDTPtr vfdt, long number) {
   float *indexes;
   DecisionTreePtr *nodes;
   int i, j;
   float thisIndex, tmpIndex;
   DecisionTreePtr cNode, tmpNode;
   VoidAListPtr growingNodes = VALNew();

   if(gMessageLevel >= 1) {
      printf("leafifying %ld nodes\n", number);
   }

   DecisionTreeGatherGrowingNodes(vfdt->dtree, growingNodes);
   indexes = MNewPtr(sizeof(float) * number);
   nodes   = MNewPtr(sizeof(DecisionTreePtr) * number);

   for(i = 0 ; i < number ; i++) {
     nodes[i] = VALIndex(growingNodes, i);
     indexes[i] = _CalculateLeafifyIndex(nodes[i], vfdt->examplesSeen);
   }

   for(i = number ; i < VALLength(growingNodes) ; i++) {
      thisIndex = _CalculateLeafifyIndex(VALIndex(growingNodes, i),
                                                      vfdt->examplesSeen);
      cNode = VALIndex(growingNodes, i);
      for(j = 0 ; j < number ; j++) {
         if(thisIndex < indexes[j]) {
            tmpIndex = indexes[j];
            indexes[j] = thisIndex;
            thisIndex = tmpIndex;
            tmpNode = nodes[j];
            nodes[j] = cNode;
            cNode = tmpNode;
         }
      }
   }

   for(i = 0 ; i < number ; i++) {
      _DoMakeLeaf(vfdt, nodes[i]);
      if(gMessageLevel >= 2) {
         printf("leafifying a node with index %f\n", indexes[i]);
      }
   }

   MFreePtr(indexes);
   MFreePtr(nodes);
   VALFree(growingNodes);
}
*/

static void _DoContinuousSplit( CVFDTPtr root, CVFDTPtr currentNode, 
				int attNum, float threshold) {
  int j;
  AttributeTrackerPtr newAT;
  int childCount;

  VFDTGrowingDataPtr parentGD = (VFDTGrowingDataPtr)
    DecisionTreeGetGrowingData(currentNode->dtreeNode);
  AttributeTrackerPtr at = 
    ExampleGroupStatsGetAttributeTracker(parentGD->egs);
  VFDTGrowingDataPtr gd;
   //  long allocation;
  int parentClass;
  float parentErrorRate;

  //  CVFDTPtr parent = currentNode;

  // Do split at decision tree level
  DecisionTreeSplitOnContinuousAttribute(currentNode->dtreeNode, attNum, threshold);
  childCount = DecisionTreeGetChildCount(currentNode->dtreeNode);
  
  // Wrap each new child as a cvfdt node
  for ( j = 0; j < childCount; j++ ) {
    CVFDTAddChild( currentNode, DecisionTreeGetChild( currentNode->dtreeNode, j ));
  }

  // update numGrowing
  root->vfdt->numGrowing--;
  root->vfdt->numGrowing += childCount;

  parentClass = ExampleGroupStatsGetMostCommonClass(parentGD->egs);
  parentErrorRate =
    (1.0 - (ExampleGroupStatsGetMostCommonClassCount(parentGD->egs) /
	    (float)ExampleGroupStatsNumExamplesSeen(parentGD->egs)));

  // children
  for ( j = 0; j < 2; j++ ) {
    newAT = AttributeTrackerClone(at);

    gd = MNewPtr(sizeof(VFDTGrowingData));
    gd->egs = ExampleGroupStatsNew(root->vfdt->spec, newAT);
    gd->parentClass = parentClass;
    //gd->splitConfidence = parentSplitConfidence;
    gd->exampleCache = 0;
    gd->parentErrorRate = parentErrorRate;
    if ( j == 0 )
      gd->seenExamplesCount = 
	ExampleGroupStatsGetPercentBelowThreshold( parentGD->egs,
						   attNum, threshold )
	* parentGD->seenExamplesCount;
    else
      gd->seenExamplesCount = 
	(1 - ExampleGroupStatsGetPercentBelowThreshold( parentGD->egs,
							attNum, threshold ))
	* parentGD->seenExamplesCount;
    gd->seenSinceLastProcess = 0;
    DecisionTreeSetGrowingData(DecisionTreeGetChild(currentNode->dtreeNode, j), gd);
    DecisionTreeSetClass(DecisionTreeGetChild(currentNode->dtreeNode, j),
			 parentClass);
  }

}

static void _DoDiscreteSplit( CVFDTPtr root,
                              CVFDTPtr currentNode, int attNum) {
  int i, j;
  AttributeTrackerPtr newAT;
  int childCount;
  
  VFDTGrowingDataPtr parentGD = (VFDTGrowingDataPtr)(DecisionTreeGetGrowingData(currentNode->dtreeNode));
  AttributeTrackerPtr at =
    ExampleGroupStatsGetAttributeTracker(parentGD->egs);
  VFDTGrowingDataPtr gd;
  //long allocation;
  int parentClass;
  
  DecisionTreeSplitOnDiscreteAttribute(currentNode->dtreeNode, attNum);
  childCount = DecisionTreeGetChildCount(currentNode->dtreeNode);
  for ( j = 0; j < childCount; j++ ) {
    CVFDTAddChild( currentNode, DecisionTreeGetChild( currentNode->dtreeNode, j ));
  }
  
  root->vfdt->numGrowing--;
  root->vfdt->numGrowing += childCount;
  
  parentClass = ExampleGroupStatsGetMostCommonClass(parentGD->egs);
  
  for(i = 0 ; i < childCount ; i++) {
    newAT = AttributeTrackerClone(at);
    AttributeTrackerMarkInactive(newAT, attNum);
    
    gd = MNewPtr(sizeof(VFDTGrowingData));
    gd->egs = ExampleGroupStatsNew(root->vfdt->spec, newAT);
    
    if ( parentGD->seenExamplesCount == 0 )
      gd->seenExamplesCount = 0;
    else
      gd->seenExamplesCount 
	= (long)(ExampleGroupStatsGetValuePercent(parentGD->egs,
						  attNum, i) * parentGD->seenExamplesCount);
    
    gd->seenSinceLastProcess = 0;
    
    DecisionTreeSetGrowingData(DecisionTreeGetChild(currentNode->dtreeNode, i), gd);
    DecisionTreeSetClass(DecisionTreeGetChild(currentNode->dtreeNode, i),
			 parentClass);
  }

  //ExampleGroupStatsFree(((VFDTGrowingDataPtr)DecisionTreeGetGrowingData(currentNode))->egs);
  //MFreePtr(DecisionTreeGetGrowingData(currentNode));
  
  /* allocation = MGetTotalAllocation();
  if(root->vfdt->maxAllocationBytes < allocation) {
    // HERE may want to control how many to leafify with a flag
    // _LeafifyLeastImportantNodes(root->vfdt, (long)(root->vfdt->numGrowing * .2));
      }*/
}

static int _ValidateSplitEntropy(CVFDTPtr root, 
                                 CVFDTPtr currentNode ) {
   float hoeffdingBound;
   int i;
   float range;
   float *allIndexes;
   float firstIndex, secondIndex;
   int firstAttrib = -1, secondAttrib = -1;
   float firstThresh = 10000, secondThresh = 10000;
   float tmpIndexOne, tmpIndexTwo, tmpThreshOne, tmpThreshTwo;
   int numChildren = 0;
   ExampleGroupStatsPtr egs 
     = ((VFDTGrowingDataPtr)DecisionTreeGetGrowingData(currentNode->dtreeNode))->egs;
   int bestSplitAtt = currentNode->dtreeNode->splitAttribute;

   // Select best two attributes
   firstIndex = secondIndex = 10000;
   allIndexes 
     = MNewPtr(sizeof(float) * ExampleSpecGetNumAttributes(root->vfdt->spec));
   for(i = 0 ; i < ExampleSpecGetNumAttributes(root->vfdt->spec) ; i++) {
     // Discrete
     if(ExampleSpecIsAttributeDiscrete(root->vfdt->spec, i)) {
       tmpIndexTwo = 10000;
       tmpIndexOne = ExampleGroupStatsEntropyDiscreteAttributeSplit(egs, i);
     // Continuous
     } else { 
       ExampleGroupStatsEntropyContinuousAttributeSplit(egs, i, &tmpIndexOne, &tmpThreshOne,
							&tmpIndexTwo, &tmpThreshTwo);
     }

     allIndexes[i] = tmpIndexOne;

     if(tmpIndexOne < firstIndex) {
       secondIndex = firstIndex;
       secondThresh = firstThresh;
       secondAttrib = firstAttrib;

       firstIndex = tmpIndexOne;
       firstThresh = tmpThreshOne;
       firstAttrib = i;

       if(tmpIndexTwo < secondIndex) {
	 secondIndex = tmpIndexTwo;
	 secondThresh = tmpThreshTwo;
	 secondAttrib = i;
       }
     } else if(tmpIndexOne < secondIndex) {
       secondIndex = tmpIndexOne;
       secondThresh = tmpThreshOne;
       secondAttrib = i;
     }
   }
   
   if(root->vfdt->messageLevel >= 3) {
     printf("Validating split, best two indices are:\n");
     printf("   Index: %f Thresh: %f Attrib: %d\n", firstIndex, firstThresh, firstAttrib);
     printf("   Index: %f Thresh: %f Attrib: %d\n", secondIndex, secondThresh, secondAttrib);
   }
   
   range = lg(ExampleSpecGetNumClasses(root->vfdt->spec));

   /* HERE, this will crash if splitConfidence is 0 */
   hoeffdingBound = (float)(sqrt(((range * range) * log(1/root->vfdt->splitConfidence)) / 
				 (2.0 * ExampleGroupStatsNumExamplesSeen(egs))));

   if ( currentNode->children != 0 )
      numChildren = VALLength( currentNode->children );
   if (( numChildren != 0 ) && ( hoeffdingBound != 0 )) {
     /*
       if ( secondIndex - firstIndex <= hoeffdingBound &&
       ExampleGroupStatsEntropyTotal(egs) > firstIndex) {
       if ( firstAttrib != currentNode->dtreeNode->splitAttribute 
       && (( secondIndex - firstIndex ) >= 0.1 * hoeffdingBound )) {
       if ( gMessageLevel >= 2 ) {
       printf("Split not valid - difference %f, bound %f at node %ld\n", 
       (secondIndex - firstIndex), hoeffdingBound, currentNode->uid );
       }
       bestSplitAtt = firstAttrib;
       }
       }*/
     if ( secondIndex - firstIndex > hoeffdingBound 
	  && ExampleGroupStatsEntropyTotal(egs) > firstIndex 
	  && firstAttrib != currentNode->dtreeNode->splitAttribute ) {
       bestSplitAtt = firstAttrib;
       
     }
     else if ( hoeffdingBound < root->vfdt->tieConfidence &&
	       ExampleGroupStatsEntropyTotal(egs) > firstIndex) {
       if ( firstAttrib != currentNode->dtreeNode->splitAttribute 
	    && (( secondIndex - firstIndex ) >= ( 0.5 * gTieConfidence ))) {
	 if ( gMessageLevel >= 2 ) {
	   printf("Node %ld was tied, difference %f\n", currentNode->uid,
		  secondIndex - firstIndex);
	 }
	 bestSplitAtt = firstAttrib;
       }
     }
   }
   MFreePtr(allIndexes);
   return bestSplitAtt;
}

static void _CheckSplitEntropy(CVFDTPtr root, 
			       CVFDTPtr currentNode ) {
   float hoeffdingBound;
   int i;
   float range;
   float *allIndexes;
   float firstIndex, secondIndex;
   int firstAttrib = -1, secondAttrib = -1;
   float firstThresh = -1, secondThresh = -1;
   float tmpIndexOne, tmpIndexTwo, tmpThreshOne, tmpThreshTwo;
//   int numChildren = 0;
   ExampleGroupStatsPtr egs 
     = ((VFDTGrowingDataPtr)DecisionTreeGetGrowingData(currentNode->dtreeNode))->egs;

   // Select best two attributes
   firstIndex = secondIndex = 10000;
   allIndexes 
     = MNewPtr(sizeof(float) * ExampleSpecGetNumAttributes(root->vfdt->spec));
   for(i = 0 ; i < ExampleSpecGetNumAttributes(root->vfdt->spec) ; i++) {
     // Discrete
     if(ExampleSpecIsAttributeDiscrete(root->vfdt->spec, i)) {
       tmpIndexTwo = 10000;
       tmpIndexOne = ExampleGroupStatsEntropyDiscreteAttributeSplit(egs, i);
     // Continuous
     } else { 
       ExampleGroupStatsEntropyContinuousAttributeSplit(egs, i, &tmpIndexOne, &tmpThreshOne,
							&tmpIndexTwo, &tmpThreshTwo);
     }

     allIndexes[i] = tmpIndexOne;

     if(tmpIndexOne < firstIndex) {
       secondIndex = firstIndex;
       secondThresh = firstThresh;
       secondAttrib = firstAttrib;

       firstIndex = tmpIndexOne;
       firstThresh = tmpThreshOne;
       firstAttrib = i;

       if(tmpIndexTwo < secondIndex) {
	 secondIndex = tmpIndexTwo;
	 secondThresh = tmpThreshTwo;
	 secondAttrib = i;
       }
     } else if(tmpIndexOne < secondIndex) {
       secondIndex = tmpIndexOne;
       secondThresh = tmpThreshOne;
       secondAttrib = i;
     }
   }
   
   if(root->vfdt->messageLevel >= 3) {
     printf("Considering split, best two indices are:\n");
     printf("   Index: %f Thresh: %f Attrib: %d\n", firstIndex, firstThresh, firstAttrib);
     printf("   Index: %f Thresh: %f Attrib: %d\n", secondIndex, secondThresh, secondAttrib);
   }
   
   range = (float)(lg(ExampleSpecGetNumClasses(root->vfdt->spec)));
   /* HERE, this will crash if splitConfidence is 0 */
   hoeffdingBound = (float)(sqrt(((range * range) * log(1/root->vfdt->splitConfidence)) / 
				 (2.0 * ExampleGroupStatsNumExamplesSeen(egs))));
   
   if(secondIndex - firstIndex > hoeffdingBound &&
      ExampleGroupStatsEntropyTotal(egs) > firstIndex) {
     if(gMessageLevel >= 1) {
       if(ExampleSpecIsAttributeDiscrete(root->vfdt->spec, firstAttrib)) {
	 printf("Did discrete split on attrib %s index %f, second best %s index %f, hoeffding %f, based on %ld examples, presplit entropy %f\n",
		ExampleSpecGetAttributeName(root->vfdt->spec, firstAttrib), 
		firstIndex, 
		ExampleSpecGetAttributeName(root->vfdt->spec, secondAttrib),
		secondIndex, hoeffdingBound,
		ExampleGroupStatsNumExamplesSeen(egs),
		ExampleGroupStatsEntropyTotal(egs));
       } else {
	 printf("Did continuous split on attrib %s thresh %f index %f, second best %s thresh %f index %f, hoeffding %f, based on %ld examples, presplit entropy %f\n",
		ExampleSpecGetAttributeName(root->vfdt->spec, firstAttrib),
		firstThresh, firstIndex, 
		ExampleSpecGetAttributeName(root->vfdt->spec, secondAttrib),
		secondThresh, secondIndex, hoeffdingBound,
                     ExampleGroupStatsNumExamplesSeen(egs),
		ExampleGroupStatsEntropyTotal(egs));
       }
       printf("   allocation now: %ld\n", MGetTotalAllocation());
     }

     if(root->vfdt->messageLevel >= 2) {
       ExampleGroupStatsWrite(egs, stdout);
     }

     if(ExampleSpecIsAttributeDiscrete(root->vfdt->spec, firstAttrib)) {
       _DoDiscreteSplit(root, currentNode, firstAttrib);
     } else {
       _DoContinuousSplit(root, currentNode, firstAttrib, firstThresh);
     }
     
     if(gMessageLevel >= 3) {
       printf("The new tree is: \n");
       CVFDTPrint(root, stdout);
     }

   } else if(hoeffdingBound < root->vfdt->tieConfidence &&
	     ExampleGroupStatsEntropyTotal(egs) > firstIndex) {
     if(gMessageLevel >= 1) {
       printf("Called it a tie at node %ld between attrib %d entropy %f, and %d entropy %f, hoeffding %f, based on %ld examples\n",
	      currentNode->uid, firstAttrib, firstIndex, 
	      secondAttrib, secondIndex, hoeffdingBound,
	      ExampleGroupStatsNumExamplesSeen(egs));
       printf("   allocation now: %ld\n", MGetTotalAllocation());
     }

     if(ExampleSpecIsAttributeDiscrete(root->vfdt->spec, firstAttrib)) {
       _DoDiscreteSplit(root, currentNode, firstAttrib);
     } else {
       _DoContinuousSplit(root, currentNode, firstAttrib, firstThresh);
     }
   } else {
     if(gMessageLevel >= 3) {
       printf("Didn't split, top entropies were %f at %d and %f at %d hoeffding %f\n",
	      firstIndex, firstAttrib, secondIndex, secondAttrib, hoeffdingBound);
       ExampleGroupStatsWrite(egs, stdout);
     }

     /* now disable any that aren't promising */
     for(i = 0 ; i < ExampleSpecGetNumAttributes(root->vfdt->spec) ; i++) {
       if(allIndexes[i] > firstIndex + hoeffdingBound
	  && i != firstAttrib && i != secondAttrib) {
	 ExampleGroupStatsIgnoreAttribute(egs, i);
	 if(gMessageLevel >= 3) {
	   printf("started ignoring an attrib %d index %f, best %f based on %ld examples\n", i, allIndexes[i], firstIndex, ExampleGroupStatsNumExamplesSeen(egs));
	 }
       }
     }
   }
   MFreePtr(allIndexes);
}

static void _AddExampleToNode(CVFDTPtr root, 
                              CVFDTPtr currentNode,
			      int level,
                              CVFDTExamplePtr ce,
			      int count);

static void _RebuildSubtree(CVFDTPtr subtreeRoot, CVFDTPtr root, CVFDTExamplePtr ce,
			    int count) 
{
   int foundNode = 0;
   int modifyNode = 0;
   CVFDTPtr currentNode = root;
   int level = 0;
   while(!foundNode && currentNode != 0) {
     if ( currentNode == subtreeRoot )
       modifyNode = 1;
     if ( modifyNode )
       _AddExampleToNode(root, currentNode, level, ce, count);
     if(DecisionTreeIsLeaf(currentNode->dtreeNode )) 
       foundNode = 1;		
     else if(DecisionTreeIsNodeGrowing(currentNode->dtreeNode )) 
       foundNode = 1;
     
     if(!foundNode) {
       currentNode = CVFDTOneStepClassify(currentNode, ce->example );
       level++;
     }
   }
}

void _CVFDTAltInfoFree( CVFDTAltInfoPtr altInfo, int freeSubtree );

void CVFDTFree( CVFDTPtr cvfdt )
{
  int i;
//  long nodeid = cvfdt->uid;
  VFDTGrowingDataPtr gd;
  if ( cvfdt->children != 0 ) {
    for ( i = 0; i < VALLength( cvfdt->children ); i++ ) {
      CVFDTPtr curChild = VALIndex( cvfdt->children, i );
      if ( curChild != 0 )
	CVFDTFree( curChild );
    }

  }
  else {
    cvfdt->vfdt->numGrowing--;
  }
  
  if ( cvfdt->altSubtreeInfos != 0 ) {
    CVFDTAltInfoPtr oldTree;
    CVFDTAltInfoPtr altTree = VALIndex( cvfdt->altSubtreeInfos, 1 );
    _CVFDTAltInfoFree( altTree, 1 );
    oldTree = VALIndex( cvfdt->altSubtreeInfos, 0 );
    _CVFDTAltInfoFree( oldTree, 0 );
    VALFree( cvfdt->altSubtreeInfos );
  }
  
  gd = (VFDTGrowingDataPtr)DecisionTreeGetGrowingData( cvfdt->dtreeNode );
  ExampleGroupStatsFree(gd->egs);
  MFreePtr( (VFDTGrowingDataPtr)gd );
  MFreePtr( cvfdt->dtreeNode );
  if (cvfdt->children != 0)
    VALFree( cvfdt->children );
  MFreePtr( cvfdt );
}


void _CVFDTExampleFree( CVFDTExamplePtr ce ) {
  ExampleFree( ce->example );
  MFreePtr( ce );
}

void _CVFDTExampleWrite( FILE* file, CVFDTExamplePtr ce ) {
  ExampleWrite( ce->example, file );
  fprintf( file, "%ul", ce->oldestNode );
}

CVFDTAltInfoPtr _CVFDTAltInfoNew( CVFDTPtr subtree ) {
  CVFDTAltInfoPtr catp = MNewPtr(sizeof(CVFDTAltInfo));
  catp->status = 0;
  catp->exampleCount = 0;
  catp->numTested = 0;
  catp->numErrors = 0;
  catp->subtree = subtree;
  catp->closestErr = 100.0;
  return catp;
}

void _CVFDTAltInfoFree( CVFDTAltInfoPtr altInfo, int freeSubtree ) {
  if ( freeSubtree )
    CVFDTFree( altInfo->subtree );
  MFreePtr(altInfo);
}

CVFDTExamplePtr _CVFDTExampleNew( ExamplePtr e ) {
  CVFDTExamplePtr ce = MNewPtr(sizeof(CVFDTExample));
  ce->example = e;
  ce->oldestNode = 0;
  ce->inQNode = 0;
  return ce;
}

CVFDTExamplePtr _CVFDTExampleRead( FILE* file, ExampleSpecPtr es  ) {
  ExamplePtr e = ExampleRead( file, es );
  CVFDTExamplePtr ce = _CVFDTExampleNew( e );
  int oldestNode;
  fscanf( file, "%ul", &oldestNode );
  ce->oldestNode = oldestNode;
  return ce;
}

void _PrimeAlternateTree( CVFDTPtr root, CVFDTPtr curNode, int count ) {
  int i, j, numExamples;
  CVFDTExamplePtr ce;
  FILE* curFile;
  char* fileName;
  int numCacheFiles = VALLength( gCacheFiles );

  for ( j = 0; j < gCurCachePos; j++ ) {
    ce = VALIndex( gCache, j );
    _RebuildSubtree(curNode, root, ce, count + j);
  }
  if (gLastFilledCacheIndex >= 0) {
    int fileIndex;
    if (gCurCacheIndex == 0) {
      if (gLastFilledCacheIndex < 4)
	return;
      fileIndex = gLastFilledCacheIndex;
    }      
    else {
      fileIndex = (gCurCacheIndex - 5 + numCacheFiles)%numCacheFiles;
    }
    for ( i = 0; i < 5; i++) {
      fileName = VALIndex( gCacheFiles, fileIndex );
      curFile = fopen( fileName, "r" );
      fscanf( curFile, "%d ", &numExamples );
      for ( j = 0; j < numExamples; j++ ) {
	ce = _CVFDTExampleRead( curFile, curNode->vfdt->spec );
	_RebuildSubtree( curNode, root, ce, count + j );
	_CVFDTExampleFree( ce );
      }
      fileIndex++;
      fileIndex = fileIndex % numCacheFiles;
      fclose( curFile );
    }
  }    
}


void _StartAlternateTree( CVFDTPtr root, CVFDTPtr parent, int nodeIndex, CVFDTPtr curNode, int attNum) {  
  //int i;
  VFDTGrowingDataPtr parentGD;
  AttributeTrackerPtr at;
  AttributeTrackerPtr newAT;
  VFDTGrowingDataPtr newGD;
  int parentClass;
  DecisionTreePtr dt;
  CVFDTAltInfoPtr newAltInfo;
  CVFDTPtr newRoot;
  CVFDTAltInfoPtr oldAltInfo;

  if ( parent->altSubtreeInfos != 0 ) {
    return;
  }

  // Place current tree in parent's alt tree info
  gNumNewAlts++;
  parent->altSubtreeInfos = VALNew();
  oldAltInfo =_CVFDTAltInfoNew( curNode );
  VALAppend( parent->altSubtreeInfos, oldAltInfo );

  // Create alternate tree
  if(gMessageLevel >= 1) {	
    printf("Starting alternate tree at %ld\n", curNode->uid );
  }
    
  parentGD = (VFDTGrowingDataPtr)(DecisionTreeGetGrowingData( parent->dtreeNode ));
  at = ExampleGroupStatsGetAttributeTracker( parentGD->egs );
  newAT = AttributeTrackerClone(at);
  newGD = MNewPtr(sizeof(VFDTGrowingData));
  parentClass = ExampleGroupStatsGetMostCommonClass( parentGD->egs );
  dt = DecisionTreeNew( parent->dtreeNode->spec );
  newRoot = CVFDTNew( root->vfdt, dt, curNode->uid );
  newAltInfo = _CVFDTAltInfoNew( newRoot );
  newAltInfo->subtree->nodeId = curNode->nodeId;
  newGD->egs = ExampleGroupStatsNew( root->vfdt->spec, newAT );
  newGD->seenExamplesCount = 0;
  newGD->seenSinceLastProcess = 0;
  DecisionTreeSetGrowingData( newAltInfo->subtree->dtreeNode, newGD );
  VALAppend( parent->altSubtreeInfos, newAltInfo );
  
  // Cleanup old
  /*
  if ( curNode->children != 0 ) {
    for ( i = 0; i < VALLength( curNode->children ); i++ ) {
      CVFDTFree( VALIndex( curNode->children, i ));	
    }
    VALFree( curNode->children );
    curNode->children = 0;
  }
  */
  
  // Perform first split on the new tree
  if (ExampleSpecIsAttributeDiscrete(root->vfdt->spec, attNum))
    _DoDiscreteSplit( root, newAltInfo->subtree, attNum );
  else {
    float indexOne, threshOne, indexTwo, threshTwo;
    ExampleGroupStatsEntropyContinuousAttributeSplit(newGD->egs, attNum,
						     &indexOne, &threshOne,
						     &indexTwo, &threshTwo);
    _DoContinuousSplit( root, newAltInfo->subtree, attNum, threshOne );
  }

  if(gMessageLevel >= 3) {
    CVFDTPrint( root, stdout );
  }
    
  //_PrimeAlternateTree( root, newAltInfo->subtree );
  
  /*
  for ( i = 0; i < VALLength( gCacheFiles ) - 1; i++ ) {
    FILE* curFile;
    char* fileName;
    int numExamples;
    int j;
    if ( i == gCurCacheIndex )
      continue;
    if ( i > gLastFilledCacheIndex )
      break;
    fileName = VALIndex( gCacheFiles, i );
    curFile = fopen( fileName, "r" );
    fscanf( curFile, "%d ", &numExamples );
    for ( j = 0; j < numExamples; j++ ) {
      ce = _CVFDTExampleRead( curFile, curNode->vfdt->spec );
      _RebuildSubtree( curNode, root, ce );
      _CVFDTExampleFree( ce );
    }
    fclose( curFile );
   }*/
  
}

void _CVFDTCheckForQuestionableNodes( CVFDTPtr root, CVFDTPtr curParent)
{
  int i;
  CVFDTPtr curNode;
  int bestSplitAtt;
  
  // Don't traverse any further in depth, this branch hasn't changed
  // or we are at the end of the branch
  if ( !curParent->hasChanged ||  ( curParent->children == 0 )) {
    return;
  }
  
  // For each child node
  for ( i = 0; i < VALLength( curParent->children ); i++ ) {
    bestSplitAtt = -1;
    curNode = VALIndex( curParent->children, i );
    bestSplitAtt = _ValidateSplitEntropy(root, curNode );
      
    if ( bestSplitAtt != curNode->dtreeNode->splitAttribute ) {
      _StartAlternateTree( root, curParent, i, curNode, bestSplitAtt);
    }
    
    _CVFDTCheckForQuestionableNodes( root, curNode );
    curParent->hasChanged = 0;
  }
}

static void _ProcessExample(CVFDTPtr cvfdt, CVFDTExamplePtr ce, int count) {
  int foundNode = 0;
  CVFDTPtr currentNode = cvfdt;
  int level = 0;
  while(!foundNode && currentNode != 0) {
    _AddExampleToNode( cvfdt, currentNode, level, ce, count);
    if(DecisionTreeIsLeaf(currentNode->dtreeNode )) 
      foundNode = 1;
    else if(DecisionTreeIsNodeGrowing(currentNode->dtreeNode )) 
      foundNode = 1;
    
    if(!foundNode) {
      currentNode = CVFDTOneStepClassify(currentNode, ce->example);
      level++;
    }
  }
}

static void _AddExampleToNode(CVFDTPtr root,
                              CVFDTPtr currentNode,
			      int level,
                              CVFDTExamplePtr ce,
			      int count ) 
{
  VFDTGrowingDataPtr gd 
    = (VFDTGrowingDataPtr)DecisionTreeGetGrowingData(currentNode->dtreeNode);
  ExampleGroupStatsPtr egs = gd->egs;

  // Check for alternate trees
  if (currentNode->altSubtreeInfos != 0) {

    CVFDTAltInfoPtr oldInfo = VALIndex(currentNode->altSubtreeInfos, 0);
    CVFDTAltInfoPtr newInfo = VALIndex(currentNode->altSubtreeInfos, 1);
    ce->inQNode = 1;

    // Increment counts
    oldInfo->exampleCount++;
    newInfo->exampleCount++;
    // Check accuracy if testing
    if ( oldInfo->status == 1) {
      oldInfo->numTested++;
      if(ExampleGetClass(ce->example) 
	 != DecisionTreeClassify(currentNode->dtreeNode, ce->example)) {
	oldInfo->numErrors++;
      }
      newInfo->numTested++;
      if(ExampleGetClass(ce->example)
	 != DecisionTreeClassify(newInfo->subtree->dtreeNode, ce->example))
	newInfo->numErrors++;
    }

    // Now use example to build tree
    // HERE Geoff removed this else...why not, as long as the test is before
     //  the example is used for learning
    //    else

    _ProcessExample(newInfo->subtree, ce, count);

    // If it is time to switch between testing and building
    if ((oldInfo->exampleCount == gAltTestNum) 
	|| (oldInfo->exampleCount == floor(0.9 * (float)gAltTestNum))) {
      // Calculate error rate if testing
      if (oldInfo->status == 1) {
	float oldErrorRate = (float)oldInfo->numErrors / (float)oldInfo->numTested;
	float newErrorRate = (float)newInfo->numErrors / (float)newInfo->numTested; 
	float errorDiff = newErrorRate - oldErrorRate;
	//int j;
	//ExampleGroupStatsPtr egs;
	if ( gMessageLevel >= 1 ) {
	  printf("Node %ld: old %.3f on %d, new %.3f on %d\n", 
                oldInfo->subtree->uid,
                oldErrorRate, oldInfo->numTested,
                newErrorRate, newInfo->numTested);
	}

	// Replace tree if new tree is better
	if (newErrorRate < oldErrorRate) {
	  int k;
	  int childCount;
	  int found = 0;
	  if ( gMessageLevel >= 1 ) {
	    printf("Switching trees at node %ld\n", currentNode->uid);
	  }
	  childCount = VALLength( currentNode->children );
	  for (k = 0; k < childCount; k++ ) {
	    void* cur = VALIndex( currentNode->children, k );
	    if (cur == oldInfo->subtree) {
	      found = 1;
	      break;
	    }
	  }
	  DebugError( found == 0, "Alt subtree not found" );
	  //VALRemove( currentNode->children, k );
	  VALSet( currentNode->children, k, newInfo->subtree);
	  
	  found = 0;
	  childCount = DecisionTreeGetChildCount( currentNode->dtreeNode );
	  for (k = 0; k < childCount; k++ ) {
	    DecisionTreePtr childDt 
	      = DecisionTreeGetChild( currentNode->dtreeNode, k );
	    if (childDt == oldInfo->subtree->dtreeNode) {
	      found = 1;
	      break;
	    }
	  }
	  DebugError( found == 0, "Alt dtree not found" );
	  //VALRemove( currentNode->dtreeNode->children, k );
	  VALSet( currentNode->dtreeNode->children, k,
		     newInfo->subtree->dtreeNode);

	  VALFree( currentNode->altSubtreeInfos );
	  currentNode->altSubtreeInfos = 0;
	  _CVFDTAltInfoFree( oldInfo, 1 );
	  _CVFDTAltInfoFree( newInfo, 0 );
	  gSplitLevels[level + 1]++; 
	  gNumSwitches++;
	}
	else if (( newInfo->closestErr + 0.001 < errorDiff) 
		 || (oldErrorRate == 0)
		 || (newInfo->closestErr == 0 && errorDiff == 0)) {
	  if ( gMessageLevel >= 1 ) {
	    printf("Pruning alt tree at node %ld\n", currentNode->uid );
	  }
	  _CVFDTAltInfoFree( oldInfo, 0 );
	  _CVFDTAltInfoFree( newInfo, 1 );
	  VALFree( currentNode->altSubtreeInfos );
	  currentNode->altSubtreeInfos = 0;
	  gNumPrunes++;
	}
	else {
	  newInfo->closestErr = errorDiff;
	}

      }

      // If we didn't replace the tree, switch and reset counts
      if ( currentNode->altSubtreeInfos != 0 ) {
	if (oldInfo->exampleCount == gAltTestNum) {
	  oldInfo->exampleCount = 0;
	  newInfo->exampleCount = 0;
	  oldInfo->numTested = 0;
	  newInfo->numTested = 0;
	  oldInfo->numErrors = 0;
	  newInfo->numErrors = 0;
	}
	oldInfo->status = (oldInfo->status + 1) % 2;
      }
    }
  }

  /* add the information */
  ExampleGroupStatsAddExample(egs, ce->example);
  if ( currentNode->nodeId > ce->oldestNode )
    ce->oldestNode = currentNode->nodeId;
	
  currentNode->examplesSeen++;
  currentNode->hasChanged = 1;
  gd->seenSinceLastProcess++;

  // Do depth first search to check for questionable nodes
  if (( root == currentNode ) && (count % gCheckSize == 0)) {
    _CVFDTCheckForQuestionableNodes( root, root );
  }

  if(gd->seenSinceLastProcess >= root->vfdt->processChunkSize) {
    gd->seenSinceLastProcess = 0;    

    if ( !DecisionTreeIsNodeGrowing( currentNode->dtreeNode ))
      return;
    
    if(!ExampleGroupStatsIsPure(egs)) {
      _CheckSplitEntropy(root, currentNode );
    }
  }
}

static void _RemoveExampleFromNode(CVFDTPtr root, 
				   CVFDTPtr currentNode,
				   CVFDTExamplePtr ce) {
  VFDTGrowingDataPtr gd 
    = (VFDTGrowingDataPtr)DecisionTreeGetGrowingData(currentNode->dtreeNode);
  ExampleGroupStatsPtr egs = gd->egs;
  
  // Check for alternate trees
  if (currentNode->altSubtreeInfos != 0) {
    //CVFDTAltInfoPtr oldInfo = VALIndex(currentNode->altSubtreeInfos, 0);
    CVFDTAltInfoPtr newInfo = VALIndex(currentNode->altSubtreeInfos, 1);
    // HERE Geoff took this out, I think it means that you will forget
     //  different examples than you learned on depending on the test period
     // instead now we test on 10% of examples but we also use them for
     // training
    //if ( oldInfo->status == 1 )
    //  return;
    CVFDTForgetExample(newInfo->subtree, ce);
  }

  /* remove the information */
  currentNode->hasChanged = 1;
  if ( currentNode->examplesSeen > 0 ) {
    ExampleGroupStatsRemoveExample(egs, ce->example);
    currentNode->examplesSeen--;
  }
}

void CVFDTForgetExample( CVFDTPtr cvfdt, CVFDTExamplePtr ce ) {
  int foundNode = 0;
  CVFDTPtr currentNode = cvfdt;
  CVFDTPtr parentNode = 0;
  int lastRemoved;
  
  while(!foundNode && currentNode != 0) {
    if ( ce->oldestNode < currentNode->nodeId )
      return;
    lastRemoved = 0;
    if ( currentNode->examplesSeen == 1 )
      lastRemoved = 1;
    _RemoveExampleFromNode(cvfdt, currentNode, ce );
    
    if(DecisionTreeIsLeaf(currentNode->dtreeNode )) 
      foundNode = 1;
    else if(DecisionTreeIsNodeGrowing(currentNode->dtreeNode )) 
      foundNode = 1;
    
    if(!foundNode) {
      parentNode = currentNode;
      currentNode = CVFDTOneStepClassify(currentNode, ce->example );
    }
  }
}

void _CVFDTProcessExamples(CVFDTPtr cvfdt, FILE *input) 
{
   ExamplePtr e = ExampleRead(input, cvfdt->vfdt->spec);
   CVFDTExamplePtr ce = _CVFDTExampleNew( e );
   //int replaceHere = 0;
   int count = 0;
   long allocation;
   DecisionTreePtr dt;
   //int cacheCount = 0;
   char* fileName;
   long learnTime = 0;
   int numQExamples = 0;
   long cacheTime = 0;
   long seen = 0;

   struct tms starttime;
   struct tms endtime;


   times(&starttime);
   gCache = VALNew();
   while(e != 0 ) {
     int i = 0;
     cvfdt->vfdt->examplesSeen++;
     seen++;
     ce->inQNode = 0;

     if(gIncrementalReporting) {
       times(&endtime);
       learnTime += endtime.tms_utime - starttime.tms_utime;
   
       _doIncrementalTest(cvfdt->vfdt, cvfdt->vfdt->spec, e);

       if(seen % 1000 == 0) {
	 _doIncrementalReport();
       }

       times(&starttime);
     }

     _ProcessExample(cvfdt, ce, count);

     if ( ce->inQNode == 1 )
       numQExamples++;
     times(&endtime);
     learnTime += endtime.tms_utime - starttime.tms_utime;
     times(&starttime);
     VALInsert( gCache, ce, gCurCachePos );
     gCurCachePos++;
     count++;
     if (( gCurCachePos == gCacheSize ) || ( count % gWindowSize == 0 )) {
       CVFDTExamplePtr curCe;
       FILE* curFile;
       fileName = VALIndex( gCacheFiles, gCurCacheIndex );
       curFile = fopen( fileName, "w" );
       fprintf( curFile, "%d ", VALLength( gCache ));
       for ( i = 0; i < VALLength( gCache ); i++ ) {
	 curCe = (CVFDTExamplePtr)VALIndex( gCache, i );
	 _CVFDTExampleWrite( curFile, curCe );
	 _CVFDTExampleFree( curCe );
       }
       VALFree( gCache );
       gCache = VALNew();
       fclose( curFile );
       if ( count < gWindowSize )
	 gLastFilledCacheIndex = gCurCacheIndex;
       if ( ++gCurCacheIndex == VALLength( gCacheFiles ))
	 gCurCacheIndex = 0;
       gCurCachePos = 0;
     }
     times(&endtime);
     cacheTime += endtime.tms_utime - starttime.tms_utime;
     times(&starttime);
     /*
     if(count % 10000 == 0){
       printf("processed %ld examples\n", count);
     }
     */
     if ( count >=  gWindowSize ) {
       if ( gCurCachePos == 0 ) {
	 CVFDTExamplePtr curCe;
	 FILE* curFile;
	 int numExamples;
	 int returnVal;
	 times(&endtime);
	 learnTime += endtime.tms_utime - starttime.tms_utime;
	 times(&starttime);	 
	 fileName = VALIndex( gCacheFiles, gCurCacheIndex );
	 curFile = fopen( fileName, "r" );
	 returnVal = fscanf( curFile, "%d ", &numExamples );
	 for ( i = 0; i < numExamples; i++ ) {
	   curCe = _CVFDTExampleRead( curFile, cvfdt->vfdt->spec );
	   VALAppend( gCache, curCe );
	 }
	 
	 fclose( curFile );
	 times(&endtime);
	 cacheTime += endtime.tms_utime - starttime.tms_utime;
	 times(&starttime);	 
       }
       ce = VALIndex( gCache, gCurCachePos );
       CVFDTForgetExample( cvfdt, ce );
       VALRemove( gCache, gCurCachePos );
       _CVFDTExampleFree( ce );
     }
     e = ExampleRead(input, cvfdt->vfdt->spec);
     ce = _CVFDTExampleNew( e );
     
     if (gUseSchedule && count == gScheduleCount) {
       gScheduleCount *= gScheduleMult;
       times(&endtime);
       learnTime += endtime.tms_utime - starttime.tms_utime;
       allocation = MGetTotalAllocation();
       dt = CVFDTGetLearnedTree( cvfdt );
       DebugMessage(1, 2, "doing tests...\n");
       _doTests( cvfdt->vfdt->spec, dt, cvfdt->vfdt->numGrowing, 
		 count, learnTime, cacheTime, 
		 allocation, 0, numQExamples,
		 gNumSwitches, gNumPrunes );
       numQExamples = 0;
       gNumSwitches = 0;
       gNumPrunes = 0;
       gNumNewAlts = 0;
       DecisionTreeFree(dt);
       times(&starttime);
     }
   }
   
   times(&endtime);
   learnTime += endtime.tms_utime - starttime.tms_utime;

   if(gIncrementalReporting) {
      _doIncrementalReport();
   }

   if(gMessageLevel >= 1) {
     printf("finished with all the examples, there are %ld growing nodes\n", 
	    cvfdt->vfdt->numGrowing);

     printf("time %.2lfs\n", ((double)learnTime) / 100);
   }
   
}

void CVFDTProcessExamples( CVFDTPtr cvfdt, FILE *input ) {
  _CVFDTProcessExamples(cvfdt, input);
}

static void _CVFDTPrintSpaces(FILE *out, int num) {
  int i;
  
  for(i = 0 ; i < num ; i++) {
    fprintf(out, " ");
  }
}

static void _CVFDTPrintHelp( CVFDTPtr cvfdt, FILE *out, int indent ) 
{
  int i;

  _CVFDTPrintSpaces(out, indent);
  if(cvfdt->dtreeNode->nodeType == dtnLeaf) {
    fprintf(out, "%ld (%d): (leaf: %s)\n", 
	    cvfdt->uid, cvfdt->examplesSeen, 
	    ExampleSpecGetClassValueName(cvfdt->dtreeNode->spec, 
					 cvfdt->dtreeNode->myclass));
  } 
  else if(cvfdt->dtreeNode->nodeType == dtnDiscrete) {
    fprintf(out, "%ld (%d): (split on %s:\n", cvfdt->uid, cvfdt->examplesSeen, 
	    ExampleSpecGetAttributeName(cvfdt->dtreeNode->spec, 
					cvfdt->dtreeNode->splitAttribute) );
    for(i = 0 ; i < VALLength(cvfdt->children) ; i++) {
      _CVFDTPrintSpaces(out, indent + 1);
      fprintf(out, "Value %s\n", 
	      ExampleSpecGetAttributeValueName(cvfdt->dtreeNode->spec, 
					       cvfdt->dtreeNode->splitAttribute, i));
      _CVFDTPrintHelp(VALIndex(cvfdt->children, i), out, indent + 2);
    }
    _CVFDTPrintSpaces(out, indent);
    fprintf(out, ")\n");

    if (cvfdt->altSubtreeInfos != 0) {
      CVFDTAltInfoPtr altTreeInfo;
      _CVFDTPrintSpaces(out, indent + 2);
      fprintf(out, "***************\n");
      altTreeInfo = VALIndex(cvfdt->altSubtreeInfos, 1);
      _CVFDTPrintHelp(altTreeInfo->subtree, out, indent + 2);
      _CVFDTPrintSpaces(out, indent + 2);
      fprintf(out, "***************\n");
    }
  } 
  else if(cvfdt->dtreeNode->nodeType == dtnContinuous) {
    fprintf(out, "%ld: (split on %s threshold %f: \n",
	    cvfdt->uid, ExampleSpecGetAttributeName(cvfdt->dtreeNode->spec, 
						    cvfdt->dtreeNode->splitAttribute),
	    cvfdt->dtreeNode->splitThreshold);

    _CVFDTPrintSpaces(out, indent + 1);
    
    /* left child */
    fprintf(out, "< %f\n", cvfdt->dtreeNode->splitThreshold);
    _CVFDTPrintHelp(VALIndex(cvfdt->children, 0), out, indent + 2);
    
    /* right child */
    fprintf(out, ">= %f\n", cvfdt->dtreeNode->splitThreshold);
    _CVFDTPrintHelp(VALIndex(cvfdt->children, 1), out, indent + 2);
    
    _CVFDTPrintSpaces(out, indent);
    fprintf(out, ")\n");
  }
  else if ( cvfdt->dtreeNode->nodeType == dtnGrowing )
    fprintf( out, "%ld (%d): (growing)\n", cvfdt->uid, cvfdt->examplesSeen );
}

void CVFDTPrint( CVFDTPtr cvfdt, FILE *out) {
  _CVFDTPrintHelp(cvfdt, out, 0);
}

CVFDTPtr CVFDTNew( VFDTPtr vfdt,
		   DecisionTreePtr dt,
		   long uid ) {
  CVFDTPtr cvfdt = MNewPtr(sizeof(CVFDT));
  cvfdt->vfdt = vfdt;
  cvfdt->dtreeNode = dt;
  cvfdt->children = 0;
  cvfdt->examplesSeen = 0;
  cvfdt->uid = uid;
  cvfdt->nodeId = gNodeId++;
  cvfdt->hasChanged = 0;
  cvfdt->altSubtreeInfos = 0;
  return cvfdt;
}


CVFDTPtr CVFDTCreateRoot(ExampleSpecPtr spec, float splitConfidence, float tieConfidence) {
  CVFDTPtr cvfdt = MNewPtr(sizeof(CVFDT));
  cvfdt->vfdt = VFDTNew( spec, splitConfidence, tieConfidence );
  cvfdt->dtreeNode = cvfdt->vfdt->dtree;
  cvfdt->children = 0;
  cvfdt->examplesSeen = 0;
  cvfdt->uid = 1;
  cvfdt->nodeId = gNodeId++;
  cvfdt->hasChanged = 0;
  cvfdt->altSubtreeInfos = 0;
  return cvfdt;
}

void CVFDTAddChild(CVFDTPtr parent, DecisionTreePtr newChild )
{
  CVFDTPtr child = MNewPtr(sizeof(CVFDT));
  child->vfdt = parent->vfdt;
  child->dtreeNode = newChild;
  child->children = 0;
  child->hasChanged = 0;
  child->examplesSeen = 0;
  child->nodeId = gNodeId++;
  if ( parent->children == 0 )
    parent->children = VALNew();
  VALAppend( parent->children, child );
  child->uid = parent->uid * 10 + VALLength( parent->children );
  child->altSubtreeInfos = 0;
}

CVFDTPtr CVFDTOneStepClassify(CVFDTPtr cvfdt, ExamplePtr e) {
  if(cvfdt->dtreeNode->nodeType == dtnLeaf 
     || cvfdt->dtreeNode->nodeType == dtnGrowing) {
    return cvfdt;
  }
  if(cvfdt->dtreeNode->nodeType == dtnDiscrete) {
    if(ExampleIsAttributeUnknown(e, cvfdt->dtreeNode->splitAttribute)) {
      /* HERE do something smarter */
      return VALIndex(cvfdt->children, 0);
    } else {
      return VALIndex(cvfdt->children, 
		      ExampleGetDiscreteAttributeValue(e, cvfdt->dtreeNode->splitAttribute));
    }
  } else if(cvfdt->dtreeNode->nodeType == dtnContinuous) {
    if(ExampleIsAttributeUnknown(e, cvfdt->dtreeNode->splitAttribute)) {
      /* HERE do something smarter */
      return VALIndex(cvfdt->children, 0);
    } else if(ExampleGetContinuousAttributeValue(e, cvfdt->dtreeNode->splitAttribute) >=
	      cvfdt->dtreeNode->splitThreshold) {
      return VALIndex(cvfdt->children, 1);
    } else { /* the value is < the threshold */
      return VALIndex(cvfdt->children, 0);
    }      
  }
  
  return cvfdt;
}

DecisionTreePtr CVFDTGetLearnedTree(CVFDTPtr cvfdt) {
  int i;
  VoidAListPtr list = VALNew();
  ExampleGroupStatsPtr egs;
  DecisionTreePtr growNode;
  DecisionTreePtr finalTree = DecisionTreeClone(cvfdt->dtreeNode);
  
  DecisionTreeGatherGrowingNodes(finalTree, list);
  
  for(i = VALLength(list) - 1 ; i >= 0 ; i--) {
    growNode = VALIndex(list, i);
    DecisionTreeSetTypeLeaf(growNode);
    
    egs = ((VFDTGrowingDataPtr)DecisionTreeGetGrowingData(growNode))->egs;
    DecisionTreeSetClass(growNode, ExampleGroupStatsGetMostCommonClass(egs));
  }

  VALFree(list);
  
  return finalTree;
}
