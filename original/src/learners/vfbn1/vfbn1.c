#include "vfml.h"
#include <stdio.h>
#include <string.h>

#include <assert.h>

#include <sys/times.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#define kOneOverE (0.367879441)

char *gFileStem      = "DF";
char *gStartingNet   = "";
int  gUseStartingNet = 0;

char *gNetOutput     = "";
int  gOutputNet      = 0;

char *gSourceDirectory = ".";
int   gDoTests = 0;

float gDelta           = 0.00001;
float gDeltaStar       = 0.00001;
float gTau             = 0.001;
int   gChunk           = 10000;
long  gLimitBytes      = -1;
double gLimitSeconds   = -1;

int   gStdin           = 0;
int   gNoReverse       = 0;

int   gCacheTestSet    = 1;
int   gDoBatch         = 0;
double gKappa          = 0.5;


int   gUseHeuristicBound = 1;
int   gUseNormalApprox   = 0;

int   gOnlyEstimateParameters = 0;

int   gMaxSearchSteps    = -1;
int   gMaxParentsPerNode = -1;
int   gMaxParameterGrowthMult = -1;
long  gMaxParameterCount = -1;

int   gUseStructuralPriorInTie = 0;

int   gSeed              = -1;


/* hack globals */
int   gP0Multiplier      = 1;
long  gNumBoundsUsed     = 0;
long  gNumCheckPointBoundsUsed = 0;
double gObservedP0       = 1;
double gEntropyRangeNet  = 0;
long  gInitialParameterCount = 0;
long  gBranchFactor      = 0;
long  gMaxBytesPerModel  = 0;
BeliefNet gPriorNet      = 0;

long  gSamplesNeeded     = 0;


static void _printUsage(char  *name) {
   printf("%s : 'Very Fast Belief Net' structure learning\n", name);

   printf("-f <filestem>\t Set the name of the dataset (default DF)\n");
   printf("-source <dir>\t Set the source data directory (default '.')\n");
   printf("-startFrom <filename>\t use net in <filename> as starting point,\n\t\t must be BIF file (default start from empty net)\n");
   printf("-outputTo <filename>\t output the learned net to <filename>\n");
   printf("-delta <prob> \t Allowed chance of error in each decision\n\t\t (default 0.00001 that's .001 percent)\n");
   printf("-tau <tie error>\t Call a tie when score might change < than\n\t\t tau percent. (default 0.001)\n");
   printf("-chunk <count> \t accumulate 'count' examples before testing for\n\t\t a winning search step(default 10000)\n");
   printf("-limitMegs <count>\t Limit dynamic memory allocation to 'count'\n\t\t megabytes, don't consider networks that take too much\n\t\t space (default no limit)\n");
   printf("-limitMinutes <count>\t Limit the run to <count> minutes then\n\t\t return current model (default no limit)\n");
   printf("-normal \t Use normal bound (default Hoeffding)\n");
   //printf("-correctBound \t Use the correct bound (default heuristic bound)\n");
   printf("-stdin \t\t Reads training examples from stdin instead of from\n\t\t <stem>.data causes a 2 second delay to help give\n\t\t input time to setup (default off)\n");
   printf("-noReverse \t Doesn't reverse links to make nets for next search\n\t\t step (default reverse links)\n");
  printf("-parametersOnly\t Only estimate parameters for current\n\t\t structure, no other learning\n");
   printf("-seed <s>\t Seed for random numbers (default random)\n");
   printf("-maxSearchSteps <num>\tLimit to <num> search steps (default no max).\n");
   printf("-maxParentsPerNode <num>\tLimit each node to <num> parents\n\t\t (default no max).\n");
   printf("-maxParameterGrowthMult <mult>\tLimit net to <mult> times starting\n\t\t # of parameters (default no max).\n");
   printf("-maxParameterCount <count>\tLimit net to <count> parameters\n\t\t (default no max).\n");
   printf("-kappa <kappa>  the structure prior penalty for batch (0 - 1), 1 is\n\t\t no penalty (default 0.5)\n");
   printf("-structureTie  Use the structural prior penalty in ties (default don't)\n");
   printf("-batch \t Run in batch mode, repeatedly scan disk, don't do hoeffding\n\t\t bounds (default off).\n");
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
      } else if(!strcmp(argv[i], "-startFrom")) {
         gStartingNet = argv[i+1];
         gUseStartingNet = 1;
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-outputTo")) {
         gNetOutput = argv[i+1];
         gOutputNet = 1;
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-source")) {
         gSourceDirectory = argv[i+1];
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-u")) {
         gDoTests = 1;
      } else if(!strcmp(argv[i], "-v")) {
         DebugSetMessageLevel(DebugGetMessageLevel() + 1);
      } else if(!strcmp(argv[i], "-structureTie")) {
         gUseStructuralPriorInTie = 1;
      } else if(!strcmp(argv[i], "-h")) {
         _printUsage(argv[0]);
         exit(0);
      } else if(!strcmp(argv[i], "-delta")) {
         sscanf(argv[i+1], "%f", &gDelta);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-kappa")) {
         sscanf(argv[i+1], "%lf", &gKappa);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-tau")) {
         sscanf(argv[i+1], "%f", &gTau);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-chunk")) {
         sscanf(argv[i+1], "%d", &gChunk);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-limitMegs")) {
         sscanf(argv[i+1], "%ld", &gLimitBytes);
         gLimitBytes *= 1024 * 1024;
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-limitMinutes")) {
         sscanf(argv[i+1], "%lf", &gLimitSeconds);
         gLimitSeconds *= 60;
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-maxSearchSteps")) {
         sscanf(argv[i+1], "%d", &gMaxSearchSteps);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-maxParentsPerNode")) {
         sscanf(argv[i+1], "%d", &gMaxParentsPerNode);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-maxParameterGrowthMult")) {
         sscanf(argv[i+1], "%d", &gMaxParameterGrowthMult);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-maxParameterCount")) {
         sscanf(argv[i+1], "%ld", &gMaxParameterCount);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-stdin")) {
         sleep(2);
         gStdin = 1;
      } else if(!strcmp(argv[i], "-normal")) {
         gUseNormalApprox = 1;
         gUseHeuristicBound = 1;
      } else if(!strcmp(argv[i], "-correctBound")) {
         gUseHeuristicBound = 0;
      } else if(!strcmp(argv[i], "-parametersOnly")) {
         gOnlyEstimateParameters = 1;
         gDoBatch = 1;
      } else if(!strcmp(argv[i], "-noReverse")) {
         gNoReverse = 1;
      } else if(!strcmp(argv[i], "-seed")) {
         sscanf(argv[i+1], "%d", &gSeed);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-noCacheTestSet")) {
         gCacheTestSet = 0;
      } else if(!strcmp(argv[i], "-batch")) {
	 gDoBatch = 1;
      } else {
         printf("Unknown argument: %s.  use -h for help\n", argv[i]);
         exit(0);
      }
   }

   DebugMessage(1, 1, "Stem: %s\n", gFileStem);
   DebugMessage(1, 1, "Source: %s\n", gSourceDirectory);
   DebugMessage(!gDoBatch, 1, "Delta: %.13f\n", gDelta);
   DebugMessage(!gDoBatch, 1, "Tau: %f\n", gTau);

   DebugMessage(gStdin, 1, "Reading examples from standard in.\n");
   DebugMessage(!gDoBatch, 1, 
             "Gather %d examples before checking for winner\n", gChunk);

   DebugMessage(gDoTests, 1, "Running tests\n");
}


VoidAListPtr _testSet;
int          _testCacheInited = 0;
static void _doTests(ExampleSpecPtr es, BeliefNet bn, long learnCount, long learnTime, long allocation, int finalOutput) {
   int oldPool = MGetActivePool();
   char fileNames[255];
   FILE *exampleIn;
   ExamplePtr e;
   long i;
   long tested, errors;

//   struct tms starttime;
//   struct tms endtime;

   errors = tested = 0;

   /* don't track this allocation against other VFBN stuff */
   MSetActivePool(0);

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
//            if(ExampleGetClass(e) != DecisionTreeClassify(dt, e)) {
//               errors++;
//            }
         }
      }
   } else {
      sprintf(fileNames, "%s/%s.test", gSourceDirectory, gFileStem);
      exampleIn = fopen(fileNames, "r");
      DebugError(exampleIn == 0, "Unable to open the .test file");
      
      DebugMessage(1, 1, "opened test file, starting scan...\n");

      e = ExampleRead(exampleIn, es);
      while(e != 0) {
         if(!ExampleIsClassUnknown(e)) {
            tested++;
//            if(ExampleGetClass(e) != DecisionTreeClassify(dt, e)) {
//               errors++;
//            }
         }
         ExampleFree(e);
         e = ExampleRead(exampleIn, es);
      }
      fclose(exampleIn);
   }

//   if(finalOutput) {
//      DebugMessage(1, 1, 
//           printf("Tested %ld examples made %ld errors\n", (long)tested,
//               (long)errors);
//      }
//      printf("%.4f\t%ld\n", ((float)errors/(float)tested) * 100,
//                (long)DecisionTreeCountNodes(dt));
//   } else {
//      printf(">> %ld\t%.4f\t%ld\t%ld\t%.2lf\t%.2f\n",
//                learnCount,
//                ((float)errors/(float)tested) * 100,
//                (long)DecisionTreeCountNodes(dt),
//                growingNodes,
//                ((double)learnTime) / 100,
//                ((double)allocation) / (1024 * 1024));
//   }
//   fflush(stdout); 

   MSetActivePool(oldPool);
}

typedef struct BNUserData_ {
   BeliefNetNode changedOne;
   BeliefNetNode changedTwo;

   /* -1 for removal, 0 for nothing, 1 for reverse, 2 for add */
   int changeComplexity;

   double scoreRange;
} BNUserDataStruct, *BNUserData;


typedef struct BNNodeUserData_ {
   double         avgDataLL;
   double         score;
   double         upperBound;
   double         lowerBound;
   double         p0;
   int            isChangedFromCurrent;
   struct BNNodeUserData_ *current; // null if this is a current node
} BNNodeUserDataStruct, *BNNodeUserData;

static void _FreeUserData(BeliefNet bn) {
   int i;
   BeliefNetNode bnn;

   for(i = 0 ; i < BNGetNumNodes(bn) ; i++) {
      bnn = BNGetNodeByID(bn, i);
      
      MFreePtr(BNNodeGetUserData(bnn));
   }

   MFreePtr(BNGetUserData(bn));
}

static void _InitUserData(BeliefNet bn, BeliefNet current) {
   int i;
   BeliefNetNode bnn, currentNode;
   BNNodeUserData data;
   BNUserData netData;

   for(i = 0 ; i < BNGetNumNodes(bn) ; i++) {
      bnn = BNGetNodeByID(bn, i);
      
      data = MNewPtr(sizeof(BNNodeUserDataStruct));
      BNNodeSetUserData(bnn, data);

      data->score = 0;
      data->avgDataLL = 0;

      data->isChangedFromCurrent = 0;
      data->p0 = 1;

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
   netData->scoreRange = 0;
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


static double _GetNetScoreRange(BeliefNet bn) {
   int i;
   BeliefNetNode bnn;
   double numCPTEntries = 0;

   for(i = 0 ; i < BNGetNumNodes(bn) ; i++) {
      bnn = BNGetNodeByID(bn, i);

      numCPTEntries += BNNodeGetNumValues(bnn) * 
                       BNNodeGetNumCPTRows(bnn);
   }

   return numCPTEntries * kOneOverE;
}

long _BoundsUsedInComparison(BeliefNet first, BeliefNet second);
long _BoundsUsedInComparison(BeliefNet first, BeliefNet second) {
   BNUserData firstData = BNGetUserData(first);
   BNUserData secondData = BNGetUserData(second);
   BeliefNetNode bnn;
   int i1, i2, i;
   long numBoundsUsed;
   
   numBoundsUsed = 0;
   i1 = i2 = -1;

   if(firstData->changedOne) {
      bnn = firstData->changedOne;
      i = i1 = BNNodeGetID(bnn);

      numBoundsUsed += (BNNodeGetNumValues(bnn) *
                       BNNodeGetNumCPTRows(bnn)) + BNNodeGetNumCPTRows(bnn);
   }

   if(firstData->changedTwo) {
      bnn = firstData->changedTwo;
      i = i2 = BNNodeGetID(bnn);

      numBoundsUsed += (BNNodeGetNumValues(bnn) *
                       BNNodeGetNumCPTRows(bnn)) + BNNodeGetNumCPTRows(bnn);
   }

   if(secondData->changedOne) {
      bnn = secondData->changedOne;
      i = BNNodeGetID(bnn);
      if(i != i1 && i != i2) {
         /* don't compare same nodes twice */
         numBoundsUsed += (BNNodeGetNumValues(bnn) *
                       BNNodeGetNumCPTRows(bnn)) + BNNodeGetNumCPTRows(bnn);
      }
   }

   if(secondData->changedTwo) {
      bnn = secondData->changedTwo;
      i = BNNodeGetID(bnn);
      if(i != i1 && i != i2) {
         /* don't compare same nodes twice */
         numBoundsUsed += (BNNodeGetNumValues(bnn) *
                       BNNodeGetNumCPTRows(bnn)) + BNNodeGetNumCPTRows(bnn);
      }
   }

   return numBoundsUsed;
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

static double _CalculateCP(BeliefNetNode bnn, double event, double row) {
   /* HERE HERE Heuristic hack */
   return (event + 1.0) / (row + (float)BNNodeGetNumValues(bnn));
   //return event / row;
}

static double _GetEpsilonNormal(BeliefNet first, BeliefNet second, BeliefNet current) {
   BNUserData firstData = BNGetUserData(first);
   BNUserData secondData = BNGetUserData(second);
   BeliefNetNode bnn;
   int i, j, k, numCPTRows, n1, n2;
   double likelihood, numSamples;
   double bound;
   StatTracker st;
   
   bound = 0;


   n1 = n2 = -1;
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

      bound += StatTrackerGetNormalBound(st, gDeltaStar);
      StatTrackerFree(st);


      bnn = _BNGetNodeByID(second, current, n1);
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

      bound += StatTrackerGetNormalBound(st, gDeltaStar);
      StatTrackerFree(st);
   }

   if(firstData->changedTwo) {
      bnn = firstData->changedTwo;
      n2 = BNNodeGetID(bnn);
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

      bound += StatTrackerGetNormalBound(st, gDeltaStar);
      StatTrackerFree(st);


      bnn = _BNGetNodeByID(second, current, n2);
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

      bound += StatTrackerGetNormalBound(st, gDeltaStar);
      StatTrackerFree(st);
   }

   if(secondData->changedOne) {
      bnn = secondData->changedOne;
      if(BNNodeGetID(bnn) != n1 && BNNodeGetID(bnn) != n2) {
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

         bound += StatTrackerGetNormalBound(st, gDeltaStar);
         StatTrackerFree(st);



         bnn = _BNGetNodeByID(first, current, BNNodeGetID(bnn));
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

         bound += StatTrackerGetNormalBound(st, gDeltaStar);
         StatTrackerFree(st);
      }
   }

   if(secondData->changedTwo) {
      bnn = secondData->changedTwo;
      if(BNNodeGetID(bnn) != n1 && BNNodeGetID(bnn) != n2) {
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

         bound += StatTrackerGetNormalBound(st, gDeltaStar);
         StatTrackerFree(st);


         bnn = _BNGetNodeByID(first, current, BNNodeGetID(bnn));
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

         bound += StatTrackerGetNormalBound(st, gDeltaStar);
         StatTrackerFree(st);
      }
   }

   return bound;
}

static double _GetComparedNodesScoreRange(BeliefNet first,
                                                      BeliefNet second) {
   BNUserData firstData = BNGetUserData(first);
   BNUserData secondData = BNGetUserData(second);
   BeliefNetNode bnn;
   int i;
   double scoreRange;
   
   scoreRange = 0;

   /* HERE is this right or should it be 2x this? */

   if(firstData->changedOne) {
      bnn = firstData->changedOne;
      i = BNNodeGetID(bnn);

      scoreRange += _GetNodeScoreRange(bnn);
   }

   if(firstData->changedTwo) {
      bnn = firstData->changedTwo;
      i = BNNodeGetID(bnn);

      scoreRange += _GetNodeScoreRange(bnn);
   }

   if(secondData->changedOne) {
      bnn = secondData->changedOne;
      i = BNNodeGetID(bnn);

      scoreRange += _GetNodeScoreRange(bnn);
   }

   if(secondData->changedTwo) {
      bnn = secondData->changedTwo;
      i = BNNodeGetID(bnn);

      scoreRange += _GetNodeScoreRange(bnn);
   }

   return scoreRange;
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

   _InitUserData(bn, bn);

   //if(gDoBatch) {
   /* with pedro's new bound I don't know if we need bother with
         taking advantage of old samples, bound has to use min #
         samples in the compared nodes cause it bounds avg difference */
   BNZeroCPTs(bn);
   //}

   newBN = BNCloneNoCPTs(bn);
   _InitUserData(newBN, bn);
   VLAppend(list, newBN);

   if(gOnlyEstimateParameters) {
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

            _InitUserData(newBN, bn);
            data = BNNodeGetUserData(srcNode);
            data->isChangedFromCurrent = 1;
            netData = BNGetUserData(newBN);
            netData->changedOne = srcNode;
            netData->changeComplexity = 2;
            netData->scoreRange = _GetNetScoreRange(newBN);

            isLinkCovered = 0;

            if((gMaxParentsPerNode == -1 ||
                   BNNodeGetNumParents(srcNode) <= gMaxParentsPerNode) &&

                   (gMaxParameterGrowthMult == -1 ||
                    (BNGetNumParameters(newBN) < 
                        (gMaxParameterGrowthMult * gInitialParameterCount))) &&

                   (gMaxParameterCount == -1 ||
                    (BNNodeGetNumParameters(srcNode) <= 
                        gMaxParameterCount)) &&

             (gLimitBytes == -1 ||
              ((MGetTotalAllocation() - preAllocation) <  gMaxBytesPerModel))&&

                   !BNHasCycle(newBN)) {
               VLAppend(list, newBN);
               isLinkCovered = _IsLinkCoveredIn(dstNode, srcNode, newBN);
            } else {
//printf("Preallocation %ld current %ld limit %ld\n", preAllocation, MGetTotalAllocation(), gMaxBytesPerModel);

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

               _InitUserData(newBN, bn);
               data = BNNodeGetUserData(dstNode);
               data->isChangedFromCurrent = 1;
               netData = BNGetUserData(newBN);
               netData->changedOne = dstNode;
               netData->changeComplexity = 2;
               netData->scoreRange = _GetNetScoreRange(newBN);

               if((gMaxParentsPerNode == -1 ||
                      BNNodeGetNumParents(dstNode) <= gMaxParentsPerNode) &&

                    (gMaxParameterGrowthMult == -1 ||
                       (BNGetNumParameters(newBN) < 
                        (gMaxParameterGrowthMult * gInitialParameterCount))) &&

                   (gMaxParameterCount == -1 ||
                    (BNNodeGetNumParameters(dstNode) <= 
                        gMaxParameterCount)) &&

             (gLimitBytes == -1 ||
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

               _InitUserData(newBN, bn);
               data = BNNodeGetUserData(srcNode);
               data->isChangedFromCurrent = 1;
               netData = BNGetUserData(newBN);
               netData->changedOne = srcNode;
               netData->changeComplexity = -1;
               netData->scoreRange = _GetNetScoreRange(newBN);
            } else {
               BNNodeRemoveParent(dstNode, srcParentOfDstIndex);
               BNNodeInitCPT(dstNode);

               _InitUserData(newBN, bn);
               data = BNNodeGetUserData(dstNode);
               data->isChangedFromCurrent = 1;
               netData = BNGetUserData(newBN);
               netData->changedOne = dstNode;
               netData->changeComplexity = -1;
               netData->scoreRange = _GetNetScoreRange(newBN);
            }

            if(!BNHasCycle(newBN) &&

               (gMaxParameterGrowthMult == -1 ||
                  (BNGetNumParameters(newBN) < 
                  (gMaxParameterGrowthMult * gInitialParameterCount))) &&

               (gMaxParameterCount == -1 ||
                  (BNNodeGetNumParameters(srcNode) <= 
                   gMaxParameterCount  &&
                  BNNodeGetNumParameters(dstNode) <= 
                                         gMaxParameterCount))&&


               (gLimitBytes == -1 ||
                    ((MGetTotalAllocation() - preAllocation) <  
                                                gMaxBytesPerModel))) {
               VLAppend(list, newBN);
            } else {
               _FreeUserData(newBN);
               BNFree(newBN);
            }

            /* copy two: reverse */
            if(!gNoReverse) {
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

                  _InitUserData(newBN, bn);
                  data = BNNodeGetUserData(srcNode);
                  data->isChangedFromCurrent = 1;
                  data = BNNodeGetUserData(dstNode);
                  data->isChangedFromCurrent = 1;
                  netData = BNGetUserData(newBN);
                  netData->changedOne = srcNode;
                  netData->changedTwo = dstNode;
                  netData->changeComplexity = 1;
                  netData->scoreRange = _GetNetScoreRange(newBN);

                  isLinkCovered = _IsLinkCoveredIn(srcNode, dstNode, newBN);
               } else {
                  BNNodeRemoveParent(dstNode, srcParentOfDstIndex);
                  BNNodeAddParent(srcNode, dstNode);
                  BNNodeInitCPT(srcNode);
                  BNNodeInitCPT(dstNode);

                  _InitUserData(newBN, bn);
                  data = BNNodeGetUserData(srcNode);
                  data->isChangedFromCurrent = 1;
                  data = BNNodeGetUserData(dstNode);
                  data->isChangedFromCurrent = 1;
                  netData = BNGetUserData(newBN);
                  netData->changedOne = srcNode;
                  netData->changedTwo = dstNode;
                  netData->changeComplexity = 1;
                  netData->scoreRange = _GetNetScoreRange(newBN);

                  isLinkCovered = _IsLinkCoveredIn(dstNode, srcNode, newBN);
               }

               if((gMaxParentsPerNode == -1 ||
                      (BNNodeGetNumParents(srcNode) <= gMaxParentsPerNode &&
                       BNNodeGetNumParents(dstNode) <= gMaxParentsPerNode)) &&

                !isLinkCovered &&

             (gLimitBytes == -1 ||
              ((MGetTotalAllocation() - preAllocation) <  gMaxBytesPerModel))&&

                (gMaxParameterGrowthMult == -1 ||
                    (BNGetNumParameters(newBN) < 
                        (gMaxParameterGrowthMult * gInitialParameterCount))) &&

               (gMaxParameterCount == -1 ||
                  (BNNodeGetNumParameters(srcNode) <= 
                   gMaxParameterCount  &&
                  BNNodeGetNumParameters(dstNode) <= 
                                         gMaxParameterCount))&&

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

static void _OptimizedAddSample(BeliefNet current, VoidListPtr newNets, 
                                               ExamplePtr e) {
   BNUserData netData;
   int i;

   BNAddSample(current, e);

   for(i = 0 ; i < VLLength(newNets) ; i++) {
      netData = BNGetUserData(VLIndex(newNets, i));

      if(netData->changedOne) {
         BNNodeAddSample(netData->changedOne, e);
      }
      if(netData->changedTwo) {
         BNNodeAddSample(netData->changedTwo, e);
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

static double _MaxPLgP(double p, float numSamples) {
   double maxP, minP, bound;

   if(numSamples == 0) {
      /* return the highest value of the function */
      return 0;
   }

   bound = StatHoeffdingBoundOne(1.0, gDeltaStar, numSamples);
   maxP = min(p + bound, 1.0);
   minP = max(p - bound, 0.0);

   if(minP == 0) {
      /* plgp is 0 when p is 0, this is a max */
      return 0;
   } else {
      return max(minP * log(minP), maxP * log(maxP));
   }
}

static double _MinPLgP(double p, float numSamples) {
   double maxP, minP, bound;

   if(numSamples == 0) {
      /* return the lowest value of the function */
      return kOneOverE * log(kOneOverE);
   }

   bound = StatHoeffdingBoundOne(1.0, gDeltaStar, numSamples);
   maxP = min(p + bound, 1.0);
   minP = max(p - bound, 0.0);

   if(minP < kOneOverE && kOneOverE < maxP) {
      /* this is the min value of the function, if it is range use it */
      return kOneOverE * log(kOneOverE);
   } else if(minP == 0) {
      /* plgp is 0 when p is 0, this is a max but will mess up in log so 
            special case it by returning value of the other (which
            can't be larger than the function's max) */
      return maxP * log(maxP);
   } else {
      return min(minP * log(minP), maxP * log(maxP));
   }
}


int _SortDoublesCompare(const void *one, const void *two) {
   if(*(double *)one == *(double *)two) {
      return 0;
   } else if(*(double *)one > *(double *)two) {
      return 1;
   } else {
      return -1;
   }
}

/* should work as long as no probabilities are greater than 1/e,
   two important observations are that slope increases as move from 1/e
    and it increases faster as p moves towards 0 than towards 1 */
static double _GetMaxPLgPCPTRowSum(BeliefNetNode bnn, double *probs, 
                                  double targetPij, double pijkEpsilon) {
   double *outputValues;
   double sum, plgp, maxDelta;
   int i;

   outputValues = MNewPtr(sizeof(double) * BNNodeGetNumValues(bnn));

   /* do one pass, set output values to minimum & find the current sum */
   sum = 0;
   for(i = 0 ; i < BNNodeGetNumValues(bnn) ; i++) {
      outputValues[i] = max(probs[i] - pijkEpsilon, 0);
      sum += outputValues[i];
   }
   
   /* do another pass, moving the larger probs back up till sum is reached */
   /*   and calculate plgp */
   plgp = 0;
   for(i = BNNodeGetNumValues(bnn) - 1 ; i >= 0 ; i--) {
      if(sum < targetPij) {
         maxDelta = (probs[i] + pijkEpsilon) - outputValues[i];
         if(sum + maxDelta < targetPij) {
            outputValues[i] += maxDelta;
            sum += maxDelta;
         } else {
            /* this is the last one that needs adjusting, set it to sum
                          exactly */
            outputValues[i] += targetPij - sum;
            sum = targetPij;
         }
      }

      if(outputValues[i] > 0) {
         plgp += outputValues[i] * log(outputValues[i]);
      }
   }

   MFreePtr(outputValues);

   DebugMessage(1, 4, "  MaxPlgP for row is: %lf\n", plgp);
   return plgp;
}

/* should work as long as no probabilities are greater than 1/e,
   two important observations are that slope increases as move from 1/e
    and it increases faster as p moves towards 0 than towards 1 */
static double _GetMinPLgPCPTRowSum(BeliefNetNode bnn, double *probs, 
                                  double targetPij, double pijkEpsilon) {
   double *outputValues;
   double sum, plgp, maxDelta, neededDelta;
   int i;
   int numSliding, highestActive;

   outputValues = MNewPtr(sizeof(double) * BNNodeGetNumValues(bnn));

   /* do one pass, set output values to maximums & find the current sum */
   sum = 0;
   for(i = 0 ; i < BNNodeGetNumValues(bnn) ; i++) {
      outputValues[i] = min(probs[i] + pijkEpsilon, kOneOverE);
      sum += outputValues[i];
   }

   /* Kinda subtle here, need to move points back from the high end,
        but moving towards the low end needs to be minimized so
        move the highest back till it hits another then move both back, etc */
   numSliding = 1;
   highestActive = BNNodeGetNumValues(bnn) - 1;

   while(sum > targetPij) {
      if(numSliding - 1 == highestActive) {
         /* all sliding */
         maxDelta = min(outputValues[0], 
              outputValues[0] - (probs[0] - pijkEpsilon));
      } else { 
         maxDelta = min(outputValues[highestActive] - 
                    outputValues[highestActive - numSliding], 
          outputValues[highestActive] - (probs[highestActive] - pijkEpsilon));
      }

      neededDelta = (sum - targetPij) / (double)numSliding;

      if(neededDelta <= maxDelta) {
         for(i = highestActive ; i > highestActive - numSliding ; i--) {
            outputValues[i] -= neededDelta;
         }
         sum = targetPij;
      } else {
         /* the delta isn't enough */
         sum -= maxDelta * (double)numSliding;
         for(i = highestActive ; i > highestActive - numSliding ; i--) {
            outputValues[i] -= maxDelta;
         }

         if(outputValues[highestActive] == 
                                    probs[highestActive] - pijkEpsilon) {
            /* that's all the highest can go */
            highestActive--;
            if(numSliding > 1) {
               numSliding--;
            } /* else only one sliding so move on to the next one*/
         } else {
            /* we musta bumped into the next point down */
            numSliding++;
         }
      }
   }

   plgp = 0;
   for(i = BNNodeGetNumValues(bnn) - 1 ; i >= 0 ; i--) {
      if(outputValues[i] > 0) {
         plgp += outputValues[i] * log(outputValues[i]);
      }
   }

   MFreePtr(outputValues);
   DebugMessage(1, 4, "  MinPlgP for row is: %lf\n", plgp);
   return plgp;
}

static double _GetMaxPLgPCPTRowSumOneAbove(BeliefNetNode bnn, double *probs, 
                                  double targetPij, double pijkEpsilon) {
   double *outputValues;
   double sum, plgp, maxDelta;
   int i;

   outputValues = MNewPtr(sizeof(double) * BNNodeGetNumValues(bnn));

   /* do one pass, set output values to minimum & find the current sum */
   /* special case the big one */
   sum = 0;
   for(i = 0 ; i < BNNodeGetNumValues(bnn) - 1 ; i++) {
      outputValues[i] = max(probs[i] - pijkEpsilon, 0);
      sum += outputValues[i];
   }

   i = BNNodeGetNumValues(bnn) - 1;
   outputValues[i] = min(probs[i] + pijkEpsilon, 1.0);
   sum += outputValues[i];

   /* do another pass, moving the larger probs back up till sum is reached */
   /*   and calculate plgp, special case the biggest value */
   plgp = 0;
   for(i = BNNodeGetNumValues(bnn) - 2 ; i >= 0 ; i--) {
      if(sum < targetPij) {
         maxDelta = (probs[i] + pijkEpsilon) - outputValues[i];
         if(sum + maxDelta < targetPij) {
            outputValues[i] += maxDelta;
            sum += maxDelta;
         } else {
            /* this is the last one that needs adjusting, set it to sum
                          exactly */
            outputValues[i] += targetPij - sum;
            sum = targetPij;
         }
      }

      if(outputValues[i] > 0) {
         plgp += outputValues[i] * log(outputValues[i]);
      }
   }

   i = BNNodeGetNumValues(bnn) - 1;
   if(sum != targetPij) {
      /* something kinda odd, the small onces can't ballance the big one
             so we actually need to move the big one back to reach the
             target */

      outputValues[i] = targetPij - (sum - outputValues[i]);
   }

   if(outputValues[i] > 0) {
      plgp += outputValues[i] * log(outputValues[i]);
   }

   MFreePtr(outputValues);

   DebugMessage(1, 4, "  MaxPlgP for row is: %lf\n", plgp);
   return plgp;
}

static double _GetMinPLgPCPTRowSumOneAbove(BeliefNetNode bnn, double *probs, 
                                  double targetPij, double pijkEpsilon) {
   double *outputValues;
   double sum, plgp, maxDelta, neededDelta;
   int i;
   int numSliding, highestActive;

   outputValues = MNewPtr(sizeof(double) * BNNodeGetNumValues(bnn));

   /* do one pass, set output values to maximums & find the current sum */
   /* special case the last one */
   sum = 0;
   for(i = 0 ; i < BNNodeGetNumValues(bnn) - 1 ; i++) {
      outputValues[i] = probs[i] + pijkEpsilon;
      sum += outputValues[i];
   }

   i = BNNodeGetNumValues(bnn) - 1;
   outputValues[i] = probs[i] - pijkEpsilon;
   sum += outputValues[i];

   /* Kinda subtle here, need to move points back from the high end,
        but moving towards the low end needs to be minimized so
        move the highest back till it hits another then move both back, etc */
   numSliding = 1;
   highestActive = BNNodeGetNumValues(bnn) - 2;

   while(sum > targetPij) {
      if(numSliding - 1 == highestActive) {
         /* all sliding */
         maxDelta = min(outputValues[0], 
              outputValues[0] - (probs[0] - pijkEpsilon));
      } else { 
         maxDelta = min(outputValues[highestActive] - 
                    outputValues[highestActive - numSliding], 
          outputValues[highestActive] - (probs[highestActive] - pijkEpsilon));
      }

      neededDelta = (sum - targetPij) / (double)numSliding;

      if(neededDelta <= maxDelta) {
         for(i = highestActive ; i > highestActive - numSliding ; i--) {
            outputValues[i] -= neededDelta;
         }
         sum = targetPij;
      } else {
         /* the delta isn't enough */
         sum -= maxDelta * (double)numSliding;
         for(i = highestActive ; i > highestActive - numSliding ; i--) {
            outputValues[i] -= maxDelta;
         }

         if(outputValues[highestActive] == 
                                    probs[highestActive] - pijkEpsilon) {
            /* that's all the highest can go */
            highestActive--;
            if(numSliding > 1) {
               numSliding--;
            } /* else only one sliding so move on to the next one*/
         } else {
            /* we musta bumped into the next point down */
            numSliding++;
         }
      }
   }

   plgp = 0;
   for(i = BNNodeGetNumValues(bnn) - 1 ; i >= 0 ; i--) {
      if(outputValues[i] > 0) {
         plgp += outputValues[i] * log(outputValues[i]);
      }
   }

   MFreePtr(outputValues);
   DebugMessage(1, 4, "  MinPlgP for row is: %lf\n", plgp);
   return plgp;
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

               //logCP = log(bnn->eventCounts[j][k] / bnn->rowCounts[j]);
               logCP = log(_CalculateCP(bnn, bnn->eventCounts[j][k],
                                     bnn->rowCounts[j]));

               if(data->p0 > bnn->eventCounts[j][k] / bnn->rowCounts[j]) {
                  data->p0 = bnn->eventCounts[j][k] / bnn->rowCounts[j];
               }

               if(gObservedP0 > bnn->eventCounts[j][k] / bnn->rowCounts[j]) {
                  gObservedP0 = bnn->eventCounts[j][k] / bnn->rowCounts[j];
               }

               data->avgDataLL += prob * logCP;
            }
         }
      }
   }
}

void _UpdateNodeScore(BeliefNetNode bnn) {
   int numCPTRows;
   BNNodeUserData data;
   int j, k;
   double prob = 0, pij;
   double numSamples;
   double pijkEpsilon, pijEpsilon;
   double *probs;
   int canOptimize, canOptimizeOneAbove;

   data = BNNodeGetUserData(bnn);

   /* if this is the current node or is changed from it */
   if(data->isChangedFromCurrent || (data->current == 0)) {
      data->score = 0;
      data->upperBound = 0;
      data->lowerBound = 0;
      numCPTRows = BNNodeGetNumCPTRows(bnn);
      numSamples = BNNodeGetNumSamples(bnn);

      probs = MNewPtr(sizeof(double) * BNNodeGetNumValues(bnn));

      for(j = 0 ; j < numCPTRows ; j++) {
         /* HACK for efficiency break BNN ADT */

         if(bnn->rowCounts[j] > 0) {
            canOptimize = 1;
            canOptimizeOneAbove = -1;
            pijkEpsilon = StatHoeffdingBoundOne(1.0, gDeltaStar,
                                                     bnn->rowCounts[j]);
            pijEpsilon = StatHoeffdingBoundOne(1.0, gDeltaStar, numSamples);
         } else {
            canOptimize = 0;
            canOptimizeOneAbove = -1;
            pijEpsilon = 0;
            pijkEpsilon = 0;
         }

         for(k = 0 ; k < BNNodeGetNumValues(bnn) ; k++) {
            if(numSamples != 0) {
               prob = bnn->eventCounts[j][k] / numSamples;

               probs[k] = prob;

               if(canOptimize && (prob > kOneOverE)) {
                  canOptimize = 0;

                  if((prob - pijkEpsilon > kOneOverE) && 
                         (canOptimizeOneAbove == -1)) {
                     canOptimizeOneAbove = 1;
                  } else {
                     canOptimizeOneAbove = 0;
                     if(prob < kOneOverE) {
                        DebugMessage(1, 4, " lt 1/e and overlaps\n");
                     } else {
                        DebugMessage(1, 4, " gt 1/e and overlaps\n");
                     }
                  }
               }

               DebugMessage(1, 4,
                 "   i: %d j: %d k: %d prob: %f epsilon: %f samples: %f\n",
                   BNNodeGetID(bnn), j, k, prob, 
                   StatHoeffdingBoundOne(1.0, gDeltaStar, bnn->rowCounts[j]),
                                         bnn->rowCounts[j]);
               if(prob != 0) {
                  DebugMessage(1, 4, 
                        "   p_ijk lower %lf score %lf upper %lf samples %lf\n",
                        _MinPLgP(prob, bnn->rowCounts[j]), prob * log(prob), 
                       _MaxPLgP(prob, bnn->rowCounts[j]), bnn->rowCounts[j]);

                  data->score += prob * log(prob);

                  if(data->p0 > bnn->eventCounts[j][k] / bnn->rowCounts[j]) {
                     data->p0 = bnn->eventCounts[j][k] / bnn->rowCounts[j];
                  }

                  if(gObservedP0 > bnn->eventCounts[j][k] / 
                                                   bnn->rowCounts[j]) {
                     gObservedP0 = bnn->eventCounts[j][k] / bnn->rowCounts[j];
                  }


               }
            }
         }

         /* now do the p_ij part */
         if(numSamples != 0) {
            pij = bnn->rowCounts[j] / numSamples;
            DebugMessage(1, 4,
              "row i: %d j: %d pij: %lf epsilon: %lf samples: %f\n",
                BNNodeGetID(bnn), j, pij,
              StatHoeffdingBoundOne(1.0, gDeltaStar, numSamples), numSamples);
            if(pij != 0) {
               DebugMessage(1, 4, 
                  "   p_ij lower %lf score %lf upper %lf samples %lf\n",
               -_MaxPLgP(pij, numSamples), -pij * log(pij),
                  -_MinPLgP(pij, numSamples), numSamples);

               data->score -= pij * log(pij);
            }
         } else {
            /* no samples, just pick something */
            pij = 0;
         }

         /* need to update the bound even if there were no samples
                 or the probability was zero */
         data->upperBound -= _MinPLgP(pij, numSamples);
         data->lowerBound -= _MaxPLgP(pij, numSamples);

         /* now do another pass to get the lower & upper bound for ther row */
         if(canOptimize) {
            DebugMessage(1, 4, "Do none above bound\n");
            qsort(probs, BNNodeGetNumValues(bnn), sizeof(double),
                                          _SortDoublesCompare);

            if(numCPTRows == 1) {
               /* know pij must == 1.0 */
               data->lowerBound += _GetMinPLgPCPTRowSum(bnn, probs, 
                               1.0, pijkEpsilon);
               data->upperBound += _GetMaxPLgPCPTRowSum(bnn, probs, 
                               1.0, pijkEpsilon);
            } else {
               data->lowerBound += _GetMinPLgPCPTRowSum(bnn, probs, 
                               min(pij + pijEpsilon, 1.0), pijkEpsilon);
               data->upperBound += _GetMaxPLgPCPTRowSum(bnn, probs, 
                               max(pij - pijEpsilon, 0), pijkEpsilon);
            }

         } else if(canOptimizeOneAbove == 1) {
            DebugMessage(1, 4, "Do one above bound\n");

            qsort(probs, BNNodeGetNumValues(bnn), sizeof(double),
                                          _SortDoublesCompare);
            if(numCPTRows == 1) {
               /* know pij must == 1.0 */
               data->lowerBound += _GetMinPLgPCPTRowSumOneAbove(bnn, probs, 
                               1.0, pijkEpsilon);
               data->upperBound += _GetMaxPLgPCPTRowSumOneAbove(bnn, probs, 
                               1.0, pijkEpsilon);
            } else {
               data->lowerBound += _GetMinPLgPCPTRowSumOneAbove(bnn, probs, 
                               min(pij + pijEpsilon, 1.0), pijkEpsilon);
               data->upperBound += _GetMaxPLgPCPTRowSumOneAbove(bnn, probs, 
                               max(pij - pijEpsilon, 0), pijkEpsilon);
            }

         } else {
            DebugMessage(1, 4, "Do bad bound\n");

            for(k = 0 ; k < BNNodeGetNumValues(bnn) ; k++) {
               data->upperBound += _MaxPLgP(prob, bnn->rowCounts[j]);
               data->lowerBound += _MinPLgP(prob, bnn->rowCounts[j]);
            }
         }  
      }

      if(data->upperBound > 0) {
         data->upperBound = 0;
      }

      MFreePtr(probs);
   }
}

static void _UpdateCurrentNetScore(BeliefNet bn) {
   int i;

   for(i = 0 ; i < BNGetNumNodes(bn) ; i++) {
      if(!gUseHeuristicBound) {
         _UpdateNodeScore(BNGetNodeByID(bn, i));
      } else {
         _UpdateNodeAveDataLL(BNGetNodeByID(bn, i));
      }
   }
}

static void _UpdateNetScore(BeliefNet bn) {
   BNUserData netData = BNGetUserData(bn);

   if(netData->changedOne) {
      if(!gUseHeuristicBound) {
         _UpdateNodeScore(netData->changedOne);
      } else {
         _UpdateNodeAveDataLL(netData->changedOne);
      }
   }
   if(netData->changedTwo) {
      if(!gUseHeuristicBound) {
         _UpdateNodeScore(netData->changedTwo);
      } else {
         _UpdateNodeAveDataLL(netData->changedTwo);
      }
   }
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

   return difference * log(gKappa);
}

void _UpdateNodeBD(BeliefNetNode bnn) {
   int numCPTRows;
   BNNodeUserData data;
   int j, k;
   double numSamples;

   data = BNNodeGetUserData(bnn);

   /* if this is the current node or is changed from it */
   if(data->isChangedFromCurrent || (data->current == 0)) {
      data->avgDataLL = 0;
      numCPTRows = BNNodeGetNumCPTRows(bnn);
      numSamples = BNNodeGetNumSamples(bnn);

      for(j = 0 ; j < numCPTRows ; j++) {
         /* HACK for efficiency break BNN ADT */

         /* HERE HERE update this to use the probabilities from the
                 prior network */
         data->avgDataLL += 
            (StatLogGamma(BNNodeGetNumValues(bnn)) -
            StatLogGamma(BNNodeGetNumValues(bnn) + bnn->rowCounts[j]));

         for(k = 0 ; k < BNNodeGetNumValues(bnn) ; k++) {
            data->avgDataLL += 
               (StatLogGamma(1 + bnn->eventCounts[j][k]) -
                      StatLogGamma(1));
         }
      }
      /* now scale it by the likelihood of the node given structural prior */
      data->avgDataLL += _GetStructuralDifferenceScoreNode(bnn);
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
      //if(!gUseHeuristicBound) {
      //   _UpdateNodeScore(netData->changedOne);
      //} else {
      _UpdateNodeBD(netData->changedOne);
      //}
   }
   if(netData->changedTwo) {
      //if(!gUseHeuristicBound) {
      //   _UpdateNodeScore(netData->changedTwo);
      //} else {
      _UpdateNodeBD(netData->changedTwo);
      //}
   }
}


static double _GetNodeScore(BeliefNetNode bnn) {
   BNNodeUserData data = BNNodeGetUserData(bnn);
   
   if(!gUseHeuristicBound) {
      if(data->current == 0 || data->isChangedFromCurrent) {
         return data->score;
      } else {
         return data->current->score;
      }
   } else {
      if(data->current == 0 || data->isChangedFromCurrent) {
         return data->avgDataLL;
      } else {
         return data->current->avgDataLL;
      }
   }
}

static double _GetNodeLowestScore(BeliefNetNode bnn) {
   BNNodeUserData data = BNNodeGetUserData(bnn);
   
   if(data->current == 0 || data->isChangedFromCurrent) {
      return data->lowerBound;
   } else {
      return data->current->lowerBound;
   }
}

static double _GetNodeHighestScore(BeliefNetNode bnn) {
   BNNodeUserData data = BNNodeGetUserData(bnn);
   
   if(data->current == 0 || data->isChangedFromCurrent) {
      return data->upperBound;
   } else {
      return data->current->upperBound;
   }
}


int _IsFirstNetHigherLowerBound(BeliefNet first, BeliefNet second);
int _IsFirstNetHigherLowerBound(BeliefNet first, BeliefNet second) {
   BNUserData firstData = BNGetUserData(first);
   BNUserData secondData = BNGetUserData(second);
   int i1, i2, i;
   double scoreOne, scoreTwo;
   
   scoreOne = scoreTwo = 0;
   i1 = i2 = -1;

   if(firstData->changedOne) {
      i = i1 = BNNodeGetID(firstData->changedOne);
      scoreOne += _GetNodeLowestScore(BNGetNodeByID(first, i));
      scoreTwo += _GetNodeLowestScore(BNGetNodeByID(second, i));
   }

   if(firstData->changedTwo) {
      i = i2 = BNNodeGetID(firstData->changedTwo);
      scoreOne += _GetNodeLowestScore(BNGetNodeByID(first, i));
      scoreTwo += _GetNodeLowestScore(BNGetNodeByID(second, i));
   }

   if(secondData->changedOne) {
      i = BNNodeGetID(secondData->changedOne);
      if(i != i1 && i != i2) {
         /* don't compare same nodes twice */
         scoreOne += _GetNodeLowestScore(BNGetNodeByID(first, i));
         scoreTwo += _GetNodeLowestScore(BNGetNodeByID(second, i));
      }
   }

   if(secondData->changedTwo) {
      i = BNNodeGetID(secondData->changedTwo);
      if(i != i1 && i != i2) {
         /* don't compare same nodes twice */
         scoreOne += _GetNodeLowestScore(BNGetNodeByID(first, i));
         scoreTwo += _GetNodeLowestScore(BNGetNodeByID(second, i));
      }
   }

   return scoreOne > scoreTwo;
}


double _GetCorrectBoundEpsilon(BeliefNet first, BeliefNet second) {
   BNUserData firstData = BNGetUserData(first);
   BNUserData secondData = BNGetUserData(second);
   int i1, i2, i;
   double scoreOne, scoreTwo, epsilon;
   
   epsilon = 0;
   scoreOne = scoreTwo = 0;
   i1 = i2 = -1;

   if(firstData->changedOne) {
      i = i1 = BNNodeGetID(firstData->changedOne);
      epsilon += _GetNodeScore(BNGetNodeByID(first, i)) -
                         _GetNodeLowestScore(BNGetNodeByID(first, i));
      epsilon += _GetNodeHighestScore(BNGetNodeByID(second, i)) - 
                         _GetNodeScore(BNGetNodeByID(second, i));
   }

   if(firstData->changedTwo) {
      i = i2 = BNNodeGetID(firstData->changedTwo);
      epsilon += _GetNodeScore(BNGetNodeByID(first, i)) -
                         _GetNodeLowestScore(BNGetNodeByID(first, i));
      epsilon += _GetNodeHighestScore(BNGetNodeByID(second, i)) - 
                         _GetNodeScore(BNGetNodeByID(second, i));
   }

   if(secondData->changedOne) {
      i = BNNodeGetID(secondData->changedOne);
      if(i != i1 && i != i2) {
         /* don't compare same nodes twice */
         epsilon += _GetNodeScore(BNGetNodeByID(first, i)) -
                         _GetNodeLowestScore(BNGetNodeByID(first, i));
         epsilon += _GetNodeHighestScore(BNGetNodeByID(second, i)) - 
                         _GetNodeScore(BNGetNodeByID(second, i));
      }
   }

   if(secondData->changedTwo) {
      i = BNNodeGetID(secondData->changedTwo);
      if(i != i1 && i != i2) {
         /* don't compare same nodes twice */
         epsilon += _GetNodeScore(BNGetNodeByID(first, i)) -
                         _GetNodeLowestScore(BNGetNodeByID(first, i));
         epsilon += _GetNodeHighestScore(BNGetNodeByID(second, i)) - 
                         _GetNodeScore(BNGetNodeByID(second, i));
      }
   }

   return epsilon;
}

float _ScoreBNOptimized(BeliefNet bn);

double _FirstLowerMinusSecondUpper(BeliefNet first, BeliefNet second);
double _FirstLowerMinusSecondUpper(BeliefNet first, BeliefNet second) {
   BNUserData firstData = BNGetUserData(first);
   BNUserData secondData = BNGetUserData(second);
   int i1, i2, i;
   double scoreOne, scoreTwo;
   
   scoreOne = scoreTwo = 0;
   i1 = i2 = -1;

   if(firstData->changedOne) {
      i = i1 = BNNodeGetID(firstData->changedOne);
      scoreOne += _GetNodeLowestScore(BNGetNodeByID(first, i));
      scoreTwo += _GetNodeHighestScore(BNGetNodeByID(second, i));
   }

   if(firstData->changedTwo) {
      i = i2 = BNNodeGetID(firstData->changedTwo);
      scoreOne += _GetNodeLowestScore(BNGetNodeByID(first, i));
      scoreTwo += _GetNodeHighestScore(BNGetNodeByID(second, i));
   }

   if(secondData->changedOne) {
      i = BNNodeGetID(secondData->changedOne);
      if(i != i1 && i != i2) {
         /* don't compare same nodes twice */
         scoreOne += _GetNodeLowestScore(BNGetNodeByID(first, i));
         scoreTwo += _GetNodeHighestScore(BNGetNodeByID(second, i));
      }
   }

   if(secondData->changedTwo) {
      i = BNNodeGetID(secondData->changedTwo);
      if(i != i1 && i != i2) {
         /* don't compare same nodes twice */
         scoreOne += _GetNodeLowestScore(BNGetNodeByID(first, i));
         scoreTwo += _GetNodeHighestScore(BNGetNodeByID(second, i));
      }
   }

   DebugMessage(1, 4, "     first %p %lf low %lf second %p %lf high %lf better %d\n", 
               first, _ScoreBNOptimized(first), scoreOne, 
               second, _ScoreBNOptimized(second), scoreTwo,
                               scoreOne > scoreTwo);
   return scoreOne - scoreTwo;
}

double _GetDeltaScore(BeliefNet first, BeliefNet second) {
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

   DebugMessage(1, 4, "     first %p %lf low %lf second %p %lf high %lf better %d\n", 
               first, _ScoreBNOptimized(first), scoreOne, 
               second, _ScoreBNOptimized(second), scoreTwo,
                               scoreOne > scoreTwo);
   return scoreOne - scoreTwo;
}


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

static int _FavorBetterNetInTie(BeliefNet best, BeliefNet compare) {
   int pBest, pCompare;

   /* EFF could optimize this to be constant instead of O(n) */
   pBest = BNGetNumIndependentParameters(best);
   pCompare = BNGetNumIndependentParameters(compare);

   //DebugMessage(1, 3, "best %d compare %d\n", pBest, pCompare);
   if(pBest == pCompare) {
      /* if compare is current return 0 else return 1 */
      return !_IsCurrentNet(compare);
   }

   return 1;

   ///* favor the current net always, if not involved favor the best net */
   //return !_IsCurrentNet(compare);

   ///* pick completely randomly */
   //return RandomRange(0, 1);
}

static void _CompareNetsBoundFreeLoosers(BeliefNet current, 
                                          VoidListPtr netChoices) {
   int i, bestIndex;
   BeliefNet bestNet, compareNet;
   double deltaScore;
   double epsilon;
   double scoreRangeNode, scoreRangeNet;
   double tauNeededToQuitNet, tauNeededToQuitNode, minDelta, maxDelta;
   double minDeltaEpsilon, maxDeltaEpsilon;

   _UpdateCurrentNetScore(current);

   /* do one pass to update scores & find net with the best lower bound */
   bestNet = VLIndex(netChoices, 0);
   bestIndex = 0;
   _UpdateNetScore(bestNet);

   for(i = 1 ; i < VLLength(netChoices) ; i++) {
      compareNet = VLIndex(netChoices, i);
      _UpdateNetScore(compareNet);

      //if(!_IsFirstNetHigherLowerBound(bestNet, compareNet)) {
      if(!_IsFirstNetBetter(bestNet, compareNet)) {
         bestNet = compareNet;
         bestIndex = i;
      }
   }

   /* take out the best net to facilitate ties and stuff */
   VLRemove(netChoices, bestIndex);

   /* do another pass deactivating clear loosers */

   tauNeededToQuitNet = tauNeededToQuitNode = 0;
   minDelta = 100000;
   maxDelta = -100000;
   minDeltaEpsilon = maxDeltaEpsilon = 0;

   for(i = VLLength(netChoices) - 1 ; i >= 0 ; i--) {
      compareNet = VLIndex(netChoices, i);
      deltaScore = _GetDeltaScore(bestNet, compareNet);
      scoreRangeNet = fabs(2 * gEntropyRangeNet);
      scoreRangeNode = fabs(_GetComparedNodesScoreRange(bestNet, compareNet));

      /* HERE for missing data */
      if(!gUseHeuristicBound) {
         epsilon = _GetCorrectBoundEpsilon(bestNet, compareNet);
      } else {
         if(!gUseNormalApprox) {
            epsilon = StatHoeffdingBoundOne(scoreRangeNode, gDeltaStar, 
                BNNodeGetNumSamples(BNGetNodeByID(current, 0)));
         } else {
            epsilon = _GetEpsilonNormal(bestNet, compareNet, current);
         }
      }
      //printf("hb %lf nb %lf\n", epsilon, 
      //          _GetEpsilonNormal(bestNet, compareNet, current));

      if(minDelta > deltaScore) {
          minDelta = deltaScore;
          minDeltaEpsilon = epsilon;
      }
      if(deltaScore > maxDelta) {
         maxDelta = deltaScore;
         maxDeltaEpsilon = epsilon;
      }

      DebugMessage(1, 3, " delta %lf epsilon %lf nodeRange: %f *tau: %f\n",
             deltaScore, epsilon, scoreRangeNode, scoreRangeNode * gTau);
      
      if(deltaScore - epsilon > 0) {
         DebugMessage(1, 2, "Early freeing a net, diff %f, epsilon %f now %d left\n", 
                  deltaScore, epsilon, VLLength(netChoices));
         gNumBoundsUsed++;

         if(_IsCurrentNet(compareNet)) {
            DebugMessage(1, 2, 
              " Current best freed bound - delta %lf epsilon %lf.\n", 
                                                    deltaScore, epsilon);
         }
         _FreeUserData(compareNet);
         BNFree(VLRemove(netChoices, i));
      } else {
         /* check for tie, if the better wins on a tie (is simpler not more
             complex) then use the diffWithBound othewise, use the opposite
             of the bound, the worse lower bound and better upperbound.
          summary: tie if it guarenteed won't cost more than range * tau */

         if(fabs(deltaScore) <= epsilon &&
                       (2 * epsilon) <= (gTau * scoreRangeNode)) {
            /* that's a tie */

            DebugMessage(1, 2, "Tie diff %f epsilon %f range %f there are %d left\n",
                   deltaScore, epsilon, scoreRangeNode, VLLength(netChoices));
            DebugMessage(1, 3, "  scoreRangeNet: %f *tau: %f\n",
                      scoreRangeNode, scoreRangeNode * gTau);
            DebugMessage(1, 3, "  best %p compare %p\n", bestNet, compareNet);

            gNumBoundsUsed += 1;

            if(gUseStructuralPriorInTie) {
               if(BNGetSimStructureDifference(bestNet, gPriorNet) <=
                       BNGetSimStructureDifference(compareNet, gPriorNet)) {
                  DebugMessage(1, 1, "   Freeing worse in tie\n");
                  _FreeUserData(compareNet);
                  BNFree(VLRemove(netChoices, i));
               } else {
                  DebugMessage(1, 1, "   Freeing better in tie\n");
                  _FreeUserData(bestNet);
                  BNFree(bestNet);
                  bestNet = VLRemove(netChoices, i);
               }
            } else {
               /* use old way of handling ties */
               if(_FavorBetterNetInTie(bestNet, compareNet)) {
                  /* the better net is not more complex */

                  if(_IsCurrentNet(compareNet)) {
                     DebugMessage(1, 2,
                        " Current best freed tie - delta %lf epsilon %lf.\n",
                         deltaScore, epsilon);
                  }
                  _FreeUserData(compareNet);
                  BNFree(VLRemove(netChoices, i));
               } else {
                  /* the better net is more complex */
                  if(_IsCurrentNet(bestNet)) {
                     DebugMessage(1, 2,
                        " Current best freed tie best - delta %lf epsilon %lf.\n",
                         deltaScore, epsilon);
                  }

                  _FreeUserData(bestNet);
                  BNFree(bestNet);
                  assert(bestNet != 0);
                  bestNet = VLRemove(netChoices, i);
                  assert(bestNet != 0);
               }
            }
         } else if(fabs(deltaScore) < epsilon) {
            /* we could possibly call a tie */
            if(tauNeededToQuitNode < (2.0 * epsilon) / scoreRangeNode) {
               tauNeededToQuitNode = (2.0 * epsilon) / scoreRangeNode;
            }
            if(tauNeededToQuitNet < (2.0 * epsilon) / scoreRangeNet) {
               tauNeededToQuitNet = (2.0 * epsilon) / scoreRangeNet;
            }
         }
      }
   }

   /* Make sure to put the best net back in the list! */
   VLAppend(netChoices, bestNet);
   bestIndex = VLLength(netChoices) - 1;

   DebugMessage(1, 2, "-- To tie need tauNet: %lf tauNode: %lf\n",
             tauNeededToQuitNet, tauNeededToQuitNode);

   DebugMessage(1, 2, "   min delta %lf e %lf max delta %lf e %lf\n",
             minDelta, minDeltaEpsilon, maxDelta, maxDeltaEpsilon);
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

   if(gLimitSeconds != -1) {
      times(&endtime);
      seconds = (double)(endtime.tms_utime - starttime.tms_utime) / 100.0;

      return seconds >= gLimitSeconds;
   }

   return 0;
}

int main(int argc, char *argv[]) {
   char fileName[255];

   FILE *exampleIn;
   FILE *netOut;
   ExampleSpecPtr es;
   ExamplePtr e;

   int allDone, stepDone, searchStep, i;

   BeliefNet bn;

   long learnTime, allocation, seenTotal, seenRound;
   IntListPtr samplesPerRound;

   struct tms starttime;
   struct tms endtime;

   VoidListPtr netChoices;
   VoidListPtr previousWinners = VLNew();

   _processArgs(argc, argv);

   MSetAllocFailFunction(_AllocFailed);   

   sprintf(fileName, "%s/%s.names", gSourceDirectory, gFileStem);
   es = ExampleSpecRead(fileName);
   DebugError(es == 0, "Unable to open the .names file");

   if(gUseStartingNet) {
      bn = BNReadBIF(gStartingNet);
      if(bn == 0) {
         DebugError(1, "couldn't read net specified by -startFrom\n");
      }
      DebugMessage(1, 1, "Initial score Range: %f\n", _GetNetScoreRange(bn));
      BNZeroCPTs(bn);
   } else {
      bn = BNNewFromSpec(es);
   }

   gEntropyRangeNet = 0;
   for(i = 0 ; i < BNGetNumNodes(bn) ; i++) {
      gEntropyRangeNet += _GetNodeScoreRange(BNGetNodeByID(bn, i));
   }

   gInitialParameterCount = BNGetNumParameters(bn);
   gBranchFactor = BNGetNumNodes(bn) * BNGetNumNodes(bn);
   if(gLimitBytes != -1) {
      gMaxBytesPerModel = gLimitBytes / gBranchFactor;
      DebugMessage(1, 2, "Limit models to %.4lf megs\n", 
                     gMaxBytesPerModel / (1024.0 * 1024.0));
   }

   gPriorNet = BNClone(bn);

   RandomInit();
   /* seed */
   if(gSeed != -1) {
      RandomSeed(gSeed);
   } else {
      gSeed = RandomRange(1, 30000);
      RandomSeed(gSeed);
   }

   gSamplesNeeded = 0.5 * 
                    pow(1.0 / gTau, 2) * 
                    log((100 * BNGetNumNodes(bn)) / gDelta);

   DebugMessage(1, 1, "running with seed %d\n", gSeed);
   DebugMessage(1, 1, "allocation %ld\n", MGetTotalAllocation());
   DebugMessage(1, 1, "initial parameters %ld\n", gInitialParameterCount);

   sprintf(fileName, "%s/%s.data", gSourceDirectory, gFileStem);

   times(&starttime);

   seenTotal = 0;
   seenRound = 0;
   samplesPerRound = ILNew();
   learnTime = 0;

   if(gStdin) {
      exampleIn = stdin;
   } else {
      /* will be set (and reset per search step) in the !allDone loop */
      exampleIn = 0;
   }

   gDeltaStar = gDelta;

   allDone = 0;
   searchStep = 0;
   while(!allDone) {
      searchStep++;
      DebugMessage(1, 2, "============== Search step: %d ==============\n",
             searchStep);
      DebugMessage(1, 2, "  Total samples: %ld Last round: %ld\n",
                       seenTotal, seenRound);
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

      if(!gStdin)  {
         exampleIn = fopen(fileName, "r");
         DebugError(exampleIn == 0, "Unable to open the data file");
      }

      seenRound = 0;
      stepDone = 0;
      e = ExampleRead(exampleIn, es);
      while(!stepDone && e != 0) {
         seenTotal++;
         seenRound++;
         /* put the eg in every net choice */
         _OptimizedAddSample(bn, netChoices, e);

         /* if not batch and have seen chunk then check for winner */
         if(seenRound % gChunk == 0) {
            gNumCheckPointBoundsUsed += VLLength(netChoices);
            if(!gDoBatch) {
               _CompareNetsBoundFreeLoosers(bn, netChoices);
               if(VLLength(netChoices) == 1) {
                  stepDone = 1;
               } else {
                  DebugMessage(1, 2, 
                       "   done with check seen in round %ld, %d left\n", 
                           seenRound, VLLength(netChoices));
               }
            }

            if(_IsTimeExpired(starttime)) {
               stepDone = allDone = 1;
            }
         }


         ExampleFree(e);
         e = ExampleRead(exampleIn, es);
      } /* !stepDone && e != 0 */

      ILAppend(samplesPerRound, seenRound);

      /* if examples ran out before bound kicked in pick a winner */
      if(VLLength(netChoices) > 1) {
         DebugMessage(!gDoBatch, 1,
         "Ran out of data before finding winner, using current best of the remaining %d.\n", VLLength(netChoices));
         _CompareNetsFreeLoosers(bn, netChoices);
      }

      /* if the winner is the current one then we are all done */
      if(BNStructureEqual(bn, (BeliefNet)VLIndex(netChoices, 0))) {
         /* make sure to free all loosing choices and the netChoices list */
         allDone = 1;
      } else if(gMaxSearchSteps != -1 && searchStep >= gMaxSearchSteps) {
         DebugMessage(1, 1, "Stopped because of search step limit\n");
         allDone = 1;
      }

      /* copy all the CPTs that are only in bn into the new winner */
       /* only really needed for final output but I do it for debugging too */
      _UpdateCPTsForFrom((BeliefNet)VLIndex(netChoices, 0), bn);


      if(gOnlyEstimateParameters) {
         allDone = 1;
      }

      /* now check all previous winners */
      /* if we detect a cycle, pick the best that happens
            in the period of the cycle */
      if(!allDone) {
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


//      if(!allDone) {
      VLAppend(previousWinners, BNClone(bn));

      _FreeUserData(bn);
      BNFree(bn);
      bn = (BeliefNet)VLRemove(netChoices, 0);
      _FreeUserData(bn);
      VLFree(netChoices);
//      }

      DebugMessage(1, 2, "Current effective Delta is %lf\n", 
               gNumBoundsUsed * gDelta);

      DebugMessage(1, 1, "   Num in checkpoint %d adjusted delta %lf\n",
                                          gNumCheckPointBoundsUsed,
                    (gNumBoundsUsed  + gNumCheckPointBoundsUsed) * gDelta);

      DebugMessage(1, 2, "Current observed p0 is %lf\n", 
               gObservedP0);
      DebugMessage(1, 2, " allocation after all free %ld\n", 
                                           MGetTotalAllocation());

      /* reset file if not in stdin mode */
      if(!gStdin) {
         fclose(exampleIn);
         exampleIn = fopen(fileName, "r");
         DebugError(exampleIn == 0, "Unable to open the data file");
      }
   } /* while !allDone */

   times(&endtime);
   learnTime += endtime.tms_utime - starttime.tms_utime;

   DebugMessage(1, 1, "done learning...\n");
   DebugMessage(1, 1, "time %.2lfs\n", ((double)learnTime) / 100);

   DebugMessage(1, 1, "Total Samples: %ld\n", seenTotal);
   if(DebugGetMessageLevel() >= 1) {
      DebugMessage(1, 1, "Samples per round:\n");
      for(i = 0 ; i < ILLength(samplesPerRound) ; i++) {
         DebugMessage(1, 1, "%d %d\n", i + 1, ILIndex(samplesPerRound, i));
      }
   }

   allocation = MGetTotalAllocation();

   if(gDoTests) {
      times(&starttime);
      _doTests(es, bn, seenTotal, learnTime, allocation, 1);
   } else {
      printf("Final score: %f\n", _ScoreBN(bn));
      printf("Final net:\n");
      BNWriteBIF(bn, stdout);
   }

   if(gOutputNet) {
      netOut = fopen(gNetOutput, "w");
      BNWriteBIF(bn, netOut);
      fclose(netOut);
   }

   BNPrintStats(bn);

   BNFree(bn);
   ExampleSpecFree(es);
   DebugMessage(1, 1, "   allocation %ld\n", MGetTotalAllocation());
   return 0;
}
