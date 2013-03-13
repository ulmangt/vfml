#include "vfml.h"
#include "vfdt-engine.h"
#include <stdio.h>
#include <string.h>

#include <sys/times.h>
#include <time.h>

char *gFileStem = "DF";
char *gSourceDirectory = ".";
int   gDoTests = 0;
int   gMessageLevel = 0;

float gSplitConfidence = 0.000001;
float gTieConfidence   = 0.05;
int   gUseGini         = 0;
int   gRescans         = 1;
int   gChunk           = 300;
int   gGrowMegs        = 1000;

int   gStdin           = 0;
int   gUseSchedule     = 0;
long  gScheduleCount   = 10000;
float gScheduleMult    = 1.44;

int   gOutputTree    = 0;

int   gCacheTestSet    = 1;
int   gREPrune         = 0;
int   gCachePruneSet   = 0;

int   gCacheTrainingExamples = 1;
int   gRestartScanPeriod     = 147377 * 2;
int   gRestartLeaves         = 1;

int   gDoBatch               = 0;
int   gPrePrune              = 1;
int   gAdaptiveDelta         = 0;

VoidAListPtr gPruneSet;
float        gPruneSetPercent  = .1;
int          gPruneCacheInited = 0;


static void _printUsage(char  *name) {
   printf("%s : 'Very Fast Decision Tree' induction\n", name);

   printf("-f <filestem>\t Set the name of the dataset (default DF)\n");
   printf("-source <dir>\t Set the source data directory (default '.')\n");
   printf("-u\t\t Test the learner's accuracy on data in <stem>.test\n");

   printf("-sc <prob> \t Allowed chance of error in each decision (default 0.0000001 that's .00001 percent)\n");
   printf("-tc <tie error>\t Call a tie when hoeffding e < this. (default 0.05)\n");
   printf("-gini\t\t Use gini gain instead of information gain (default information gain) (DOESN'T WORK FOR CONTINUOUS ATTRIBUTES)\n");
   printf("-rescans <#>\t Naievely consider each example '#' times with no concern for using it once per level of the induced tree (default 1)\n");
   printf("-chunk <count> \t Wait until 'count' examples accumulate at a leaf before testing for a split (default 300)\n");
   printf("-growMegs <count>\t Limit dynamic memory allocation to 'count' megabytes (default 1000)\n");
   printf("-stdin \t\t Reads training examples from stdin instead of from <stem>.data (default off) - NOTE this disables the rescans switch\n");
   printf("-schedule <mult> Run tests after 10k examples & every mult * 10k.\n");
   printf("-outputTree \t Output the binary tree at each test point.\n");
   printf("-noCacheTestSet \t By default vfdt reads test set from disk once and keeps in memory.  Use this option to conserve some memory.\n");
   printf("-prune \t Do reduced error pruning on the resulting tree\n");
   printf("-noRestartLeaves \t Do not attempt to restart leaves that were deactivated for memory reasons.\n");
   printf("-noCacheTrainingExamples \t Do not use extra RAM to cache training examples.\n");
   printf("-batch \t Run in batch mode, read all examples into ram, don't do hoeffding bounds (default off).\n");
   printf("-noPrePrune \t Make splits that have information-loss (doesn't effect batch mode)\n");
   printf("-adaptiveDelta \t Double 'sc' each level down the induced tree until it reaches 5%\n");
   printf("-v\t\t Can be used multiple times to increase the debugging output\n");
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
      } else if(!strcmp(argv[i], "-h")) {
         _printUsage(argv[0]);
         exit(0);
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
      } else if(!strcmp(argv[i], "-growMegs")) {
         sscanf(argv[i+1], "%d", &gGrowMegs);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-stdin")) {
         gStdin = 1;
      } else if(!strcmp(argv[i], "-schedule")) {
         gUseSchedule = 1;
         sscanf(argv[i+1], "%f", &gScheduleMult);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-outputTree")) {
         gOutputTree = 1;
      } else if(!strcmp(argv[i], "-noCacheTestSet")) {
         gCacheTestSet = 0;
      } else if(!strcmp(argv[i], "-prune")) {
         gREPrune = 1;
      } else if(!strcmp(argv[i], "-noRestartLeaves")) {
         gRestartLeaves = 0;
      } else if(!strcmp(argv[i], "-noCacheTrainingExamples")) {
         gCacheTrainingExamples = 0;
      } else if(!strcmp(argv[i], "-batch")) {
	 gDoBatch = 1;
      } else if(!strcmp(argv[i], "-noPrePrune")) {
         gPrePrune = 0;
      } else if(!strcmp(argv[i], "-adaptiveDelta")) {
         gAdaptiveDelta = 1;
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
         printf("Using Gini split index - remember this won't work for continuous attributes.\n");
      } else {
         printf("Using information gain split index\n");
      }

      if(gStdin) {
         printf("Reading examples from standard in.\n");
      } else {
         printf("Considering each example %d times.\n", gRescans);
      }
      printf("Checking for splits for every %d examples at a leaf\n", gChunk);

      if(gREPrune) {
         printf("Doing reduced error pruning on resulting tree.\n");
      }

      if(gDoTests) {
         printf("Running tests\n");
      }
   }
}

VoidAListPtr _testSet;
int          _testCacheInited = 0;
static void _doTests(ExampleSpecPtr es, DecisionTreePtr dt, long growingNodes, long learnCount, long learnTime, long allocation, int finalOutput) {
   int oldPool = MGetActivePool();
   char fileNames[255];
   FILE *exampleIn, *treeOut;
   ExamplePtr e;
   long i;
   long tested, errors;

   struct tms starttime;
   struct tms endtime;

   errors = tested = 0;

   /* don't track this allocation against other VFDT stuff */
   MSetActivePool(0);

   if(gREPrune) {
      if(gCachePruneSet) {
         if(!gPruneCacheInited) {
            gPruneSet = VALNew();

            sprintf(fileNames, "%s/%s.prune", gSourceDirectory, gFileStem);
            exampleIn = fopen(fileNames, "r");
            DebugError(exampleIn == 0, "Unable to open the .prune file");

            e = ExampleRead(exampleIn, es);
            while(e != 0) {
               VALAppend(gPruneSet, e);
               e = ExampleRead(exampleIn, es);
            }
            fclose(exampleIn);
            gPruneCacheInited = 1;
         }
      } else {
         sprintf(fileNames, "%s/%s.prune", gSourceDirectory, gFileStem);
         exampleIn = fopen(fileNames, "r");
         DebugError(exampleIn == 0, "Unable to open the .prune file");
      }

      times(&starttime);

      if(gCachePruneSet) {
         REPruneBatch(dt, gPruneSet);
      } else {
         REPruneBatchFile(dt, exampleIn, 0);
         fclose(exampleIn);
      }

      times(&endtime);
      learnTime += endtime.tms_utime - starttime.tms_utime;
      if(gMessageLevel >= 1) {
         printf("Prune Time %.2lf\n",
              ((double)endtime.tms_utime - starttime.tms_utime)/100);
      }
   }


   if(gCacheTestSet) {
      if(!_testCacheInited) {
         _testSet = VALNew();

         sprintf(fileNames, "%s/%s.test", gSourceDirectory, gFileStem);
         exampleIn = fopen(fileNames, "r");
         DebugError(exampleIn == 0, "Unable to open the .test file");

         e = ExampleRead(exampleIn, es);
         while(e != 0) {
            VALAppend(_testSet, e);
            e = ExampleRead(exampleIn, es);
         }
         fclose(exampleIn);
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
      }

   } else {
      sprintf(fileNames, "%s/%s.test", gSourceDirectory, gFileStem);
      exampleIn = fopen(fileNames, "r");
      DebugError(exampleIn == 0, "Unable to open the .test file");
      
      if(gMessageLevel >= 1) {
         printf("opened test file, starting scan...\n");
      }

      e = ExampleRead(exampleIn, es);
      while(e != 0) {
         if(!ExampleIsClassUnknown(e)) {
            tested++;
            if(ExampleGetClass(e) != DecisionTreeClassify(dt, e)) {
               errors++;
            }
         }
         ExampleFree(e);
         e = ExampleRead(exampleIn, es);
      }
      fclose(exampleIn);
   }

   if(finalOutput) {
      if(gMessageLevel >= 1) {
         printf("Tested %ld examples made %ld errors\n", (long)tested,
               (long)errors);
      }
      printf("%.4f\t%ld\n", ((float)errors/(float)tested) * 100,
                (long)DecisionTreeCountNodes(dt));
   } else {
      printf("%ld\t%.4f\t%ld\t%ld\t%.2lf\t%.2f\n",
                learnCount,
                ((float)errors/(float)tested) * 100,
                (long)DecisionTreeCountNodes(dt),
                growingNodes,
                ((double)learnTime) / 100,
                ((double)allocation) / (1024 * 1024));
   }
   fflush(stdout); 

   if(gOutputTree) {
      sprintf(fileNames, "%s-%lu.tree", gFileStem, learnCount);
      treeOut = fopen(fileNames, "w");
      DecisionTreeWrite(dt, treeOut);
      fclose(treeOut);
   }

   MSetActivePool(oldPool);
}

int main(int argc, char *argv[]) {
   char fileNames[255];

   FILE *exampleIn, *pruneSet;
   ExampleSpecPtr es;
   ExamplePtr e;

   VFDTPtr vfdt;
   DecisionTreePtr dt;

   long tested, errors, seen;

   long learnTime, allocation;

   int iteration;

   struct tms starttime;
   struct tms endtime;

   _processArgs(argc, argv);

   sprintf(fileNames, "%s/%s.names", gSourceDirectory, gFileStem);
   es = ExampleSpecRead(fileNames);
   DebugError(es == 0, "Unable to open the .names file");

   RandomInit();

   /* initialize the vfdt */
   vfdt = VFDTNew(es, gSplitConfidence, gTieConfidence);
   VFDTSetUseGini(vfdt, gUseGini);
   VFDTSetProcessChunkSize(vfdt, gChunk);
   VFDTSetMaxAllocationMegs(vfdt, gGrowMegs);
   VFDTSetMessageLevel(vfdt, gMessageLevel);
   VFDTSetPrePrune(vfdt, gPrePrune);
   VFDTSetAdaptiveDelta(vfdt, gAdaptiveDelta);
   if(!gRestartLeaves) {
      VFDTSetRestartLeaves(vfdt, 0);
   }
   if(!gCacheTrainingExamples) {
      VFDTSetCacheTrainingExamples(vfdt, 0);
   }

   if(gMessageLevel >= 1) {
      printf("allocation %ld\n", MGetTotalAllocation());
   }

   if(gREPrune) {
      if(gCachePruneSet) {
         gPruneSet = VALNew();
         gPruneCacheInited = 1;
      } else {
         sprintf(fileNames, "%s/%s.prune", gSourceDirectory, gFileStem);
         pruneSet = fopen(fileNames, "w");
         DebugError(pruneSet == 0, "Unable to open the .prune file");
      }
   }

   sprintf(fileNames, "%s/%s.data", gSourceDirectory, gFileStem);

   if(gStdin) {
      gRescans = 1;
   }

   times(&starttime);

   if(gDoBatch) {
      if(gStdin) {
         exampleIn = stdin;
      } else {
         exampleIn = fopen(fileNames, "r");
         DebugError(exampleIn == 0, "Unable to open the data file");
      }

      if(gREPrune) {
         e = ExampleRead(exampleIn, es);
         while(e != 0) {
            seen++;
            if(RandomDouble() < gPruneSetPercent) {
               if(gCachePruneSet) {
                  VALAppend(gPruneSet, e);
               } else {
                  ExampleWrite(e, pruneSet);
                  ExampleFree(e);
               }
            } else {
               VFDTProcessExampleBatch(vfdt, e);
            }
            e = ExampleRead(exampleIn, es);
         }
         VFDTBatchExamplesDone(vfdt);

      } else {
         VFDTProcessExamplesBatch(vfdt, exampleIn);
      }

      if(!gStdin) {
         fclose(exampleIn);
      }

   } else {
      seen = 0;
      learnTime = 0;
      for(iteration = 0 ; iteration < gRescans ; iteration++) {
         if(gStdin) {
            exampleIn = stdin;
         } else {
            exampleIn = fopen(fileNames, "r");
            DebugError(exampleIn == 0, "Unable to open the .data file");
         }

         e = ExampleRead(exampleIn, es);
         while(e != 0) {
            seen++;

            if(gREPrune && (RandomDouble() < gPruneSetPercent)) {
               if(gCachePruneSet) {
                  VALAppend(gPruneSet, e);
               } else {
                  ExampleWrite(e, pruneSet);
                  ExampleFree(e);
               }
            } else {
               VFDTProcessExample(vfdt, e);
            }
            /* HERE if I use an example cache I better have this commented */
            //    ExampleFree(e);
            e = ExampleRead(exampleIn, es);

            /* check to see if it's time to run tests */
            if(gUseSchedule && seen == gScheduleCount) {
               gScheduleCount *= gScheduleMult;
               allocation = MGetTotalAllocation();
               times(&endtime);
               learnTime += endtime.tms_utime - starttime.tms_utime;
//printf("allocation %.2f\n", (float)allocation / (1024 * 1024));
//fflush(stdout);
               dt = VFDTGetLearnedTree(vfdt);

               if(gREPrune && !gCachePruneSet) {
                  fflush(pruneSet);
                  fclose(pruneSet);
               }
               _doTests(es, dt, VFDTGetNumGrowing(vfdt),
                      seen, learnTime, allocation, 0);

               if(gREPrune && !gCachePruneSet) {
                  sprintf(fileNames, "%s/%s.prune", 
                            gSourceDirectory, gFileStem);
                  pruneSet = fopen(fileNames, "a");
                  DebugError(pruneSet == 0, "Unable to open the .prune file");
               }

               DecisionTreeFree(dt);
               times(&starttime);
            }
         }

         if(!gStdin) {
            fclose(exampleIn);
         }
      }
   }

   times(&endtime);
   learnTime += endtime.tms_utime - starttime.tms_utime;

   if(gMessageLevel >= 1) {
      printf("done learning...\n");

      printf("   allocation %ld\n", MGetTotalAllocation());

      printf("time %.2lfs\n", ((double)learnTime) / 100);
   }

   allocation = MGetTotalAllocation();
   dt = VFDTGetLearnedTree(vfdt);

   if(gDoTests) {
      times(&starttime);
      _doTests(es, dt, VFDTGetNumGrowing(vfdt), seen, learnTime, allocation, 1);
   } else if(gUseSchedule) {
      times(&starttime);
      _doTests(es, dt, VFDTGetNumGrowing(vfdt), seen, learnTime, allocation, 0);
   } else  {
      DecisionTreePrint(dt, stdout);
   }

   if(gMessageLevel >= 1) {
      printf("allocation %ld\n", MGetTotalAllocation());

      times(&endtime);
      printf("time %.2lfs\n", ((double)(endtime.tms_utime) -
                       (double)(starttime.tms_utime)) / 100);
   }

   return 0;
}

