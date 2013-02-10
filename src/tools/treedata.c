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
int   gStdout = 0;

float gPrunePercent = 25;
int gFirstPruneLevel = 3;
int gMaxLevel = 18;
int gConceptSeed = 100;

char *gTmpFileName = "tempfile";

/* Some hack globals */
ExampleSpecPtr gEs; /* will be set to the spec for the run */
ExampleGeneratorPtr gEg;
DecisionTreePtr gDT;


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
         printf("   -conceptSeed <the multiplier for the concept seed> (default 100)\n");
         printf("   -seed <random seed> (default to random)\n");
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
      } else if(!strcmp(argv[i], "-conceptSeed")) {
         sscanf(argv[i+1], "%d", &gConceptSeed);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-v")) {
         DebugSetMessageLevel(DebugGetMessageLevel() + 1);
      } else if(!strcmp(argv[i], "-stdout")) {
         gStdout = 1;
      } else {
         printf("unknown argument '%s', use -h for help\n", argv[i]);
         exit(0);
      }
   }
}

double *_attMin;
double *_attMax;

static void _CreateConceptTreeHelper(DecisionTreePtr dt, int seed, long node, int level, AttributeTrackerPtr at) {
   int prunePercent;
   int i, activeCount, found, splitAttribute;
//   VoidListPtr kids;

   double thresh, min, max;

   RandomSeed(seed * node);

   /* test for prune */
   prunePercent = gPrunePercent;
   if(level < gFirstPruneLevel) {
      prunePercent = 0;
   }

   if(level > gMaxLevel || RandomRange(0, 100) < prunePercent
                        || AttributeTrackerAreAllInactive(at)) {
      DecisionTreeSetTypeLeaf(dt);
      DecisionTreeSetClass(dt, RandomRange(0, gNumClasses - 1));
      return;
   }

   /* now pick split attribute */
   activeCount = AttributeTrackerNumActive(at);

   splitAttribute = RandomRange(0, activeCount - 1);
   found = 0;
   for(i = 0 ; i < ExampleSpecGetNumAttributes(gEs) && !found; i++) {
      if(AttributeTrackerIsActive(at, i)) {
         if(splitAttribute == 0) {
            splitAttribute = i;
            found = 1;
         } else {
            splitAttribute--;
         }
      }
   }

   if(ExampleSpecIsAttributeDiscrete(gEs, splitAttribute)) {
      AttributeTrackerMarkInactive(at, splitAttribute);

      DecisionTreeSplitOnDiscreteAttribute(dt, splitAttribute);
      _CreateConceptTreeHelper(DecisionTreeGetChild(dt, 0), seed, node * 2, level + 1, at);
      _CreateConceptTreeHelper(DecisionTreeGetChild(dt, 1), seed, (node * 2) + 1, level + 1, at);
      AttributeTrackerMarkActive(at, splitAttribute);
   } else {
      /* must be continuous */
      thresh = (double)RandomRange(_attMin[splitAttribute] * 10000, _attMax[splitAttribute] * 10000) / (double)10000;
      DecisionTreeSplitOnContinuousAttribute(dt, splitAttribute, thresh);

      max = _attMax[splitAttribute];
      _attMax[splitAttribute] = thresh;
      if(_attMax[splitAttribute] - _attMin[splitAttribute] < 0.05) {
         AttributeTrackerMarkInactive(at, splitAttribute);
      }
      _CreateConceptTreeHelper(DecisionTreeGetChild(dt, 0), seed, node * 2, level + 1, at);
      AttributeTrackerMarkActive(at, splitAttribute);

      _attMax[splitAttribute] = max;


      min = _attMin[splitAttribute];
      _attMin[splitAttribute] = thresh;

      if(_attMax[splitAttribute] - _attMin[splitAttribute] < 0.05) {
         AttributeTrackerMarkInactive(at, splitAttribute);
      }
      _CreateConceptTreeHelper(DecisionTreeGetChild(dt, 1), seed, (node * 2) + 1, level + 1, at);
      AttributeTrackerMarkActive(at, splitAttribute);
      
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

static DecisionTreePtr _CreateConceptTree(void) {
   void *oldState;
   int seed = gConceptSeed;
   int i;

   AttributeTrackerPtr at = AttributeTrackerInitial(gEs);
   DecisionTreePtr dt;

   _attMin = MNewPtr(sizeof(double) * ExampleSpecGetNumAttributes(gEs));
   _attMax = MNewPtr(sizeof(double) * ExampleSpecGetNumAttributes(gEs));
   for(i = 0 ; i < ExampleSpecGetNumAttributes(gEs) ; i++) {
      _attMin[i] = 0;
      _attMax[i] = 1;
   }

   oldState = (void *)RandomNewState(10);

   dt = DecisionTreeNew(gEs);

   _CreateConceptTreeHelper(dt, seed, 1, 0, at);

   oldState = (void *)RandomSetState(oldState);
   RandomFreeState(oldState);
   AttributeTrackerFree(at);

   _PrunePureSubtrees(dt);

   return dt;
}

static ExampleSpecPtr _CreateExampleSpec(void) {
   ExampleSpecPtr es = ExampleSpecNew();

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
         sprintf(tmpStr, "v%d-%d", i, j);
         ExampleSpecAddAttributeValue(es, num, _AllocateString(tmpStr));
      }
   }

   for(i = 0 ; i < gNumContAttributes ; i++) {
      sprintf(tmpStr, "c-attribute %d", i);
      num = ExampleSpecAddContinuousAttribute(es, _AllocateString(tmpStr));
   }

   return es;
}

static void _makeData(void) {
   char fileNames[255];
   FILE *fileOut;
   ExamplePtr e;
   int class;
   long i;
   ExampleGeneratorPtr eg;


   double genTime, classifyTime, writeTime, totalTime;

   struct tms starttime;
   struct tms endtime;
   struct tms starttimeTotal;
   struct tms endtimeTotal;

   genTime = classifyTime = writeTime = 0;

   eg = ExampleGeneratorNew(gEs, gSeed);

   if(!gStdout) {
      sprintf(fileNames, "%s.data", gFileStem);
      fileOut = fopen(fileNames, "w");
   } else {
      fileOut = stdout;
   }

   times(&starttimeTotal);
   for(i = 0 ; i < gNumTrainExamples ; i++) {
      times(&starttime);
      e = ExampleGeneratorGenerate(eg);
      times(&endtime);
      genTime += endtime.tms_utime - starttime.tms_utime;
      genTime += endtime.tms_stime - starttime.tms_stime;


      times(&starttime);
      class = DecisionTreeClassify(gDT, e);
      times(&endtime);
      classifyTime += endtime.tms_utime - starttime.tms_utime;
      classifyTime += endtime.tms_stime - starttime.tms_stime;

      ExampleSetClass(e, class);
      ExampleAddNoise(e, gErrorLevel, 1, 1);

      times(&starttime);
      ExampleWrite(e, fileOut);
      times(&endtime);
      writeTime += endtime.tms_utime - starttime.tms_utime;
      writeTime += endtime.tms_stime - starttime.tms_stime;


      ExampleFree(e);
   }
   times(&endtimeTotal);
   totalTime = endtimeTotal.tms_utime - starttimeTotal.tms_utime;
   totalTime += endtimeTotal.tms_stime - starttimeTotal.tms_stime;

   DebugMessage(1, 2, "Gen: %.2f Classify: %.2f Write: %.2f Total: %.2f\n", genTime / 100.0, classifyTime / 100.0, writeTime / 100.0, totalTime / 100.0);


   if(!gStdout) {
      fclose(fileOut);
   }

   ExampleGeneratorFree(eg);
}

static void _makeSpecConceptTestPrune(void) {
   char fileNames[255];
   FILE *fileOut;
   ExamplePtr e;
   int class;
   long i;
   ExampleGeneratorPtr eg;


   gEs = _CreateExampleSpec();

   sprintf(fileNames, "%s.names", gFileStem);
   fileOut = fopen(fileNames, "w");
   ExampleSpecWrite(gEs, fileOut);
   fclose(fileOut);

   DebugMessage(1, 2, "Making concept...\n");
   gDT = _CreateConceptTree();

   sprintf(fileNames, "%s.concept", gFileStem);
   fileOut = fopen(fileNames, "w");
   DecisionTreePrint(gDT, fileOut);
   fclose(fileOut);

   eg = ExampleGeneratorNew(gEs, RandomRange(1, 10000));

   if(gNumPruneExamples > 0) {
      DebugMessage(1, 2, "Making prune data...\n");
      sprintf(fileNames, "%s.prune", gFileStem);
      fileOut = fopen(fileNames, "w");

      for(i = 0 ; i < gNumPruneExamples ; i++) {
         e = ExampleGeneratorGenerate(eg);
         class = DecisionTreeClassify(gDT, e);

         ExampleSetClass(e, class);
         ExampleAddNoise(e, gErrorLevel, 1, 1);
         ExampleWrite(e, fileOut);
         ExampleFree(e);
      }
      fclose(fileOut);

   }

   DebugMessage(1, 2, "Making test data...\n");
   sprintf(fileNames, "%s.test", gFileStem);
   fileOut = fopen(fileNames, "w");
   for(i = 0 ; i < gNumTestExamples ; i++) {
      e = ExampleGeneratorGenerate(eg);
      class = DecisionTreeClassify(gDT, e);

      ExampleSetClass(e, class);
      ExampleAddNoise(e, gErrorLevel, 1, 1);
      ExampleWrite(e, fileOut);
      ExampleFree(e);
   }
   fclose(fileOut);

   ExampleGeneratorFree(eg);

   sprintf(fileNames, "%s.stats", gFileStem);
   fileOut = fopen(fileNames, "w");

   fprintf(fileOut, "The concept tree has %d nodes.\n Leaves by level: ", DecisionTreeCountNodes(gDT));
   DecisionTreePrintStats(gDT, fileOut);

   fprintf(fileOut, "Num Attributes: %d discrete & %d continuous\n", gNumDiscAttributes, gNumContAttributes);
   fprintf(fileOut, "Num Classes %d\n", gNumClasses);

   fprintf(fileOut, "Num train examples %ld\n", gNumTrainExamples);
   fprintf(fileOut, "Num test examples %ld\n", gNumTestExamples);
   fprintf(fileOut, "Num prune examples %ld\n", gNumPruneExamples);
   fprintf(fileOut, "Noise level %.2f%%\n", gErrorLevel * 100);
   fprintf(fileOut, "Prune percent %.0f%%\n", gPrunePercent);
   fprintf(fileOut, "First prune level %d\n", gFirstPruneLevel);
   fprintf(fileOut, "Max level %d\n", gMaxLevel);
   fprintf(fileOut, "Example seed %ld\n", gSeed);
   fprintf(fileOut, "Concept seed %d\n", gConceptSeed);

   fclose(fileOut);
}


int main(int argc, char *argv[]) {

   _processArgs(argc, argv);

   RandomInit();

   if(gSeed != -1) {
      RandomSeed(gSeed);
   } else {
      gSeed = RandomRange(1, 30000);
      RandomSeed(gSeed);
   }
 
   _makeSpecConceptTestPrune();

   DebugMessage(1, 2, "Making train data...\n");
   _makeData();

   return 0;
}

