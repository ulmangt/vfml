#include "vfml.h"
#include <stdio.h>
#include <string.h>

#include <assert.h>

#include <sys/times.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#include "ModelSearch.h"

//#define kOneOverE (0.367879441)

char *gFileStem          = "DF";
char *gStartingNetFile   = "";
int  gUseStartingNet = 0;

char *gNetOutput     = "";
int  gOutputNet      = 0;

char *gSourceDirectory = ".";

float gDelta           = 0.000000001;
float gTau             = 0.001;
int   gChunk           = 10000;
long  gLimitBytes      = -1;
double gLimitSeconds   = -1;

int   gStdin           = 0;
int   gAllowRemove     = 1;

int   gMaxSearchSteps    = -1;
int   gMaxParentsPerNode = -1;
long  gMaxParameterCount = -1;

int   gSeed              = -1;
int   gInStep            = 0;


/* hack globals */
int   gP0Multiplier      = 1;
long  gNumBoundsUsed     = 0;
long  gNumCheckPointBoundsUsed  = 0;
double gObservedP0       = 1;
long  gInitialParameterCount = 0;
long  gBranchFactor      = 0;
long  gMaxBytesPerModel  = 0;
BeliefNet gPriorNet      = 0;
BeliefNet gCurrentNet    = 0;
ExampleSpecPtr gEs       = 0;

int gFinishedByTime      = 0;

long  gNumByTie          = 0;
long  gNumByWin          = 0;
long  gMinSamplesInDecision = 1000000;
long  gMaxSamplesInDecision = 0;
long  gTotalSamplesInDecision = 0;
long  gNumZeroSamplesInDecision = 0;
long  gNumCurrentInTie   = 0;
long  gNumCurrentByWin   = 0;
long  gNumCurrentByDefault        = 0;
long  gNumByCycleConflict         = 0;
long  gNumByParameterConflict     = 0;
long  gNumByParentLimit  = 0;
long  gNumByMemoryLimit  = 0;
long  gNumExceededMemory = 0;
long  gNumByParameterLimit  = 0;
long  gNumByChangeLimit  = 0;
long  gNumRemoved        = 0;
long  gNumAdded          = 0;
long  gNumUsedWholeDB    = 0;

static void _printUsage(char  *name) {
   printf("%s : 'Very Fast Belief Net v2' structure learning\n", name);

   printf("-f <filestem>\t Set the name of the dataset (default DF)\n");
   printf("-source <dir>\t Set the source data directory (default '.')\n");
   printf("-startFrom <filename>\t use net in <filename> as starting point, must\n\t\t be BIF file (default start from empty net)\n");
   printf("-outputTo <filename>\t output the learned net to <filename>\n");
   printf("-delta <prob> \t Allowed chance of error in each decision\n\t\t (default 0.0000000001 that's .00000001 percent)\n");
   printf("-tau <tie error>\t Call a tie when score might change < than tau\n\t\t percent (default 0.001)\n");
   printf("-chunk <count> \t accumulate 'count' examples before testing for a\n\t\t winning search step(default 10000)\n");
   printf("-noRemove \t VFBN2 won't consider removing links (default remove)\n");
   printf("-inStep \t VFBN2 won't let any search make a second step before everyone\n\t\t makes a step (default no restriction)\n");
   printf("-limitMegs <count>\t Limit dynamic memory allocation to 'count' megabytes\n\t\t networks that are too large will not be considered (default\n\t\t no limit)\n");
   printf("-limitMinutes <count>\t Limit the run to <count> minutes then return\n\t\t current model (default no limit)\n");
   printf("-stdin \t\t Reads training examples from stdin instead of from\n\t\t <stem>.data causes a 2 second delay to help give\n\t\t datagen time to setup (default off)\n");
   printf("-seed <s>\t Seed for random numbers (default random)\n");
   printf("-maxParentsPerNode <num>\tLimit each node to <num> parents\n\t\t (default no max).\n");
   printf("-maxParameterCount <count>\tLimit net to <count> parameters\n\t\t (default no max).\n");
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
         gStartingNetFile = argv[i+1];
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
      } else if(!strcmp(argv[i], "-v")) {
         DebugSetMessageLevel(DebugGetMessageLevel() + 1);
      } else if(!strcmp(argv[i], "-h")) {
         _printUsage(argv[0]);
         exit(0);
      } else if(!strcmp(argv[i], "-delta")) {
         sscanf(argv[i+1], "%f", &gDelta);
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
      } else if(!strcmp(argv[i], "-maxParameterCount")) {
         sscanf(argv[i+1], "%ld", &gMaxParameterCount);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-stdin")) {
         sleep(2);
         gStdin = 1;
      } else if(!strcmp(argv[i], "-noRemove")) {
         gAllowRemove = 0;
      } else if(!strcmp(argv[i], "-inStep")) {
         gInStep = 1;
      } else if(!strcmp(argv[i], "-seed")) {
         sscanf(argv[i+1], "%d", &gSeed);
         /* ignore the next argument */
         i++;
      } else {
         printf("Unknown argument: %s.  use -h for help\n", argv[i]);
         exit(0);
      }
   }

   DebugMessage(1, 1, "Stem: %s\n", gFileStem);
   DebugMessage(1, 1, "Source: %s\n", gSourceDirectory);
   DebugMessage(1, 1, "Delta: %.13f\n", gDelta);
   DebugMessage(1, 1, "Tau: %f\n", gTau);

   DebugMessage(gStdin, 1, "Reading examples from standard in.\n");
   DebugMessage(1, 1, 
             "Gather %d examples before checking for winner\n", gChunk);
}


#ifdef NOT_DEFINED

int oldMain(int argc, char *argv[]) {
   FILE *exampleIn;
   FILE *netOut;
   ExamplePtr e;

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



      seenRound = 0;
      stepDone = 0;
      e = ExampleRead(exampleIn, es);
      while(!stepDone && e != 0) {
         seenTotal++;
         seenRound++;
         /* put the eg in every net choice */
         _OptimizedAddSample(bn, netChoices, e);

         /* if not batch and have seen chunk then check for winner */
         if(!gDoBatch && (seenRound % gChunk == 0)) {
            _CompareNetsBoundFreeLoosers(bn, netChoices);
            if(VLLength(netChoices) == 1) {
               stepDone = 1;
            } else {
               DebugMessage(1, 2, 
                    "   done with check seen in round %ld, %d left\n", 
                        seenRound, VLLength(netChoices));

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


      if(!allDone) {
         VLAppend(previousWinners, BNClone(bn));

         _FreeUserData(bn);
         BNFree(bn);
         bn = (BeliefNet)VLRemove(netChoices, 0);
         _FreeUserData(bn);
         VLFree(netChoices);
      }

      DebugMessage(1, 2, "Current effective Delta is %lf\n", 
               gNumBoundsUsed * gDelta);
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



   DebugMessage(1, 1, "Total Samples: %ld\n", seenTotal);
   if(DebugGetMessageLevel() >= 1) {
      DebugMessage(1, 1, "Samples per round:\n");
      for(i = 0 ; i < ILLength(samplesPerRound) ; i++) {
         DebugMessage(1, 1, "%d %d\n", i + 1, ILIndex(samplesPerRound, i));
      }
   }

   allocation = MGetTotalAllocation();

   printf("Final score: %f\n", _ScoreBN(bn));
   printf("Final net:\n");
   BNWriteBIF(bn, stdout);

   if(gOutputNet) {
      netOut = fopen(gNetOutput, "w");
      BNWriteBIF(bn, netOut);
      fclose(netOut);
   }

   BNFree(bn);
   ExampleSpecFree(es);
   DebugMessage(1, 1, "   allocation %ld\n", MGetTotalAllocation());
   return 0;
}

#endif





void _AllocFailed(int allocationSize) {
   printf("MEMORY ALLOCATION FAILED, size %d\n", allocationSize);
}

static void _InitializeGlobals(int argc, char *argv[]) {
   char fileName[255];

   _processArgs(argc, argv);

   MSetAllocFailFunction(_AllocFailed);   

   if(gUseStartingNet) {
      gPriorNet = BNReadBIF(gStartingNetFile);
      if(gPriorNet == 0) {
         DebugError(1, "couldn't read net specified by -startFrom\n");
      }

      gEs = BNGetExampleSpec(gPriorNet);
   } else {
      sprintf(fileName, "%s/%s.names", gSourceDirectory, gFileStem);
      gEs = ExampleSpecRead(fileName);
      DebugError(gEs == 0, "Unable to open the .names file");

      gPriorNet = BNNewFromSpec(gEs);
   }

   gInitialParameterCount = BNGetNumParameters(gPriorNet);
   gBranchFactor = BNGetNumNodes(gPriorNet) * BNGetNumNodes(gPriorNet);
   if(gLimitBytes != -1) {
      gMaxBytesPerModel = gLimitBytes / BNGetNumNodes(gPriorNet);
      //gMaxBytesPerModel = gLimitBytes / gBranchFactor;
      DebugMessage(1, 2, "Limit models to %.4lf megs\n", 
                     gMaxBytesPerModel / (1024.0 * 1024.0));
   }

   gCurrentNet = BNClone(gPriorNet);
   BNZeroCPTs(gCurrentNet);

   RandomInit();
   /* seed */
   if(gSeed != -1) {
      RandomSeed(gSeed);
   } else {
      gSeed = RandomRange(1, 30000);
      RandomSeed(gSeed);
   }

   DebugMessage(1, 1, "running with seed %d\n", gSeed);
   DebugMessage(1, 1, "allocation %ld\n", MGetTotalAllocation());
   DebugMessage(1, 1, "initial parameters %ld\n", gInitialParameterCount);

}


static int _OverMemoryLimit(void) {
   return (gLimitBytes != -1) && (MGetTotalAllocation() > gLimitBytes);
}

static void _ActivateSearches(VoidListPtr active, VoidListPtr deactive, VoidListPtr paused) {

   /* HERE this will activate too many and not be efficient,
         would be better if we could ask the searches how much
         memory they think they are gonna take */

   if(!gInStep) {
      while(VLLength(paused) > 0) {
         VLAppend(deactive, VLRemove(paused, 0));
      }
   }

   DebugMessage(1, 3, "activate current allocation %ld\n",
                                       MGetTotalAllocation());
   while(!_OverMemoryLimit() && VLLength(deactive) > 0) {
      MSActivate(VLIndex(deactive, 0));
      VLAppend(active, VLRemove(deactive, 0));
      DebugMessage(1, 3, "  another active now allocation %ld\n",
                                          MGetTotalAllocation());
   }

   while(_OverMemoryLimit() && VLLength(active) > 1) {
      MSDeactivate(VLIndex(active, VLLength(active) - 1));
      VLPush(deactive, VLRemove(active, VLLength(active) - 1));
      DebugMessage(1, 3, 
          "  deactivated last now allocation %ld\n",
                                 MGetTotalAllocation());
   }

   if(_OverMemoryLimit() && VLLength(active) > 0) {
      DebugMessage(1, 1, "*** Memory limit exceeded by search on node %d\n",
            MSGetNodeID(VLIndex(active, 0)));
      gNumExceededMemory++;
      DebugMessage(1, 3, "  single search too large allocation %ld\n",
                                                   MGetTotalAllocation());
   }

   if(VLLength(active) == 0 && VLLength(deactive) == 0 &&
                                        VLLength(paused) > 0) {
      while(VLLength(paused) > 0) {
         VLAppend(deactive, VLRemove(paused, 0));
      }

      _ActivateSearches(active, deactive, paused);
   }
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
   int i,j;

   VoidListPtr activeSearches, inactiveSearches, pausedSearches;
   int allDone, changes;

   struct tms starttime;
   struct tms endtime;
   long learnTime;

   long seenTotal, exampleNumber;

   char dataFileName[255];
   FILE *exampleIn;
   ExamplePtr e;

   FILE *netOut;

   /* init the data */
   _InitializeGlobals(argc, argv);

   times(&starttime);

   /* set up the many searches */
   activeSearches = VLNew();
   inactiveSearches = VLNew();
   pausedSearches = VLNew();
   for(i = 0 ; i < BNGetNumNodes(gCurrentNet) ; i++) {
      VLAppend(inactiveSearches, MSNew(gCurrentNet, i));
   }
   _ActivateSearches(activeSearches, inactiveSearches, pausedSearches);

   sprintf(dataFileName, "%s/%s.data", gSourceDirectory, gFileStem);
   if(gStdin) {
      exampleIn = stdin;
   } else {
      exampleIn = fopen(dataFileName, "r");
      DebugError(exampleIn == 0, "Unable to open the data file");
   }

   seenTotal = exampleNumber = 0;
   allDone = 0;

   while(!allDone) {
      e = ExampleRead(exampleIn, gEs);
      if(e != 0) {
         exampleNumber++;
      } else {
         /* if example file is empty reset it.  if stdin, then stop */
         if(gStdin) {
            allDone = 1;
         } else {
            fclose(exampleIn);
            exampleIn = fopen(dataFileName, "r");
            DebugError(exampleIn == 0, "Unable to open the data file");
            e = ExampleRead(exampleIn, gEs);
            exampleNumber = 1;
         }
      }
      seenTotal++;

      changes = 0;
      if(!allDone) {
         /* give the eg+num to each search */
         for(i = 0 ; i < VLLength(activeSearches) ; i++) {
            MSAddExample(VLIndex(activeSearches, i), e, exampleNumber);
         }

         /* HERE maybe add to the deactivated searches to est params */

         /* if chunk time then tell them all to check for winners */
         if(seenTotal % gChunk == 0) {
            DebugMessage(1, 1, 
                  "===== check there are %d active %d inactive =====\n",
                     VLLength(activeSearches), VLLength(inactiveSearches));
            /* see if there are any changes to structure */
            for(i = VLLength(activeSearches) - 1 ; i >= 0 ; i--) {
               gNumCheckPointBoundsUsed += 
                          MSNumActiveAlternatives(VLIndex(activeSearches, i));

               if(MSIsDone(VLIndex(activeSearches, i))) {
                  /* several things could make it done, conflicts eg */
                  MSFree(VLRemove(activeSearches, i));
                  changes = 1;
               } else if(MSCheckForWinner(VLIndex(activeSearches, i))) {
                  changes = 1;
                  /* Go through all the others and check for conflicts */
                  for(j = VLLength(activeSearches) - 1 ; j >= 0 ; j--) { 
                     if(j != i) {
                       MSModelChangedHandleConflicts(
                                           VLIndex(activeSearches, j));
                       /* note this may force a winner or a no-decision */
                     }
                  }

                  if(MSIsDone(VLIndex(activeSearches, i))) {
                     MSFree(VLRemove(activeSearches, i));
                  } else {
                     VLAppend(pausedSearches, VLRemove(activeSearches, i));
                  }
               }
            }

            /* do another pass to remove done and inactive searches */
            for(i = VLLength(activeSearches) - 1 ; i >= 0 ; i--) {
               if(MSIsDone(VLIndex(activeSearches, i))) {
                  changes = 1;
                  /* several things could make it done, conflicts eg */
                  MSFree(VLRemove(activeSearches, i));
               } else if(!MSIsActive(VLIndex(activeSearches, i))) {
                  changes = 1;
                  VLAppend(pausedSearches, VLRemove(activeSearches, i));
               }
            }

            if(changes) {
               _ActivateSearches(activeSearches, inactiveSearches, 
                                                        pausedSearches);
               changes = 0;
            }

            /* if everyone is done then stop */
            if(VLLength(activeSearches) == 0) {
               allDone = 1;
            }

            if(_IsTimeExpired(starttime)) {
               allDone = 1;
               gFinishedByTime = 1;
            }
         }

      } else {
         /* will be allDone if reading from stdin and got no eg */
         /*    or if time expired */
         /* tell every search to call a tie and finish up */

         for(i = 0 ; i < VLLength(activeSearches) ; i++) {
            MSTerminateSearch(VLIndex(activeSearches, i));
         }

         /* NOTE: Ignore the non-active searches, they won't have anything to
                         add to the results and can just quit */
      }

      ExampleFree(e);
   } /* !allDone */

   /* print out final things */
   /* HERE need to get: maybe p0? */
   times(&endtime);
   learnTime = endtime.tms_utime - starttime.tms_utime;
   DebugMessage(1, 1, "done learning...\n");
   DebugMessage(1, 1, "time %.2lfs\n", ((double)learnTime) / 100);

   DebugMessage(1, 1, "Total Samples: %ld\n", seenTotal);

   if(gOutputNet) {
      netOut = fopen(gNetOutput, "w");
      BNWriteBIF(gCurrentNet, netOut);
      fclose(netOut);
   } else {
      BNWriteBIF(gCurrentNet, stdout);
   }

   BNPrintStats(gCurrentNet);

   DebugMessage(1, 1, "   Num decisions %ld by tie %ld by win %ld\n",
                     gNumBoundsUsed, gNumByTie, gNumByWin);
   DebugMessage(1, 1, 
          "   Samples in step min: %ld max: %ld avg: %ld zero# %ld\n",
              gMinSamplesInDecision, gMaxSamplesInDecision,
              gTotalSamplesInDecision / (gNumRemoved + gNumAdded),
                                      gNumZeroSamplesInDecision);
   DebugMessage(1, 1, "   Num removed %ld, Num added %ld\n",
                       gNumRemoved, gNumAdded);
   DebugMessage(1, 1, "   Num used whole DB %ld\n", gNumUsedWholeDB);
   DebugMessage(1, 1, "   Search end by win %ld in tie %ld default %ld\n", 
                     gNumCurrentByWin, gNumCurrentInTie, gNumCurrentByDefault);
   DebugMessage(1, 1, "   Elim by conflict: cycle %ld, parameter %ld\n",
                         gNumByCycleConflict, gNumByParameterConflict);
   DebugMessage(1, 1, "   Elim by parent limit %ld\n", gNumByParentLimit);
   DebugMessage(1, 1, "   Elim by parameter limit %ld\n",
                                           gNumByParameterLimit);
   DebugMessage(1, 1, "   Elim by memory limit %ld\n", gNumByMemoryLimit);
   DebugMessage(1, 1, "   Elim by change limit %ld\n", gNumByChangeLimit);
   DebugMessage(1, 1, "   Num single searches too big for memory %ld\n",
                                        gNumExceededMemory); 
   DebugMessage(1, 1, "   Current effective Delta is %lf\n",
               gNumBoundsUsed * gDelta);
   DebugMessage(1, 1, "   Num in checkpoint %ld adjusted delta %lf\n",
                                          gNumCheckPointBoundsUsed,
                    (gNumBoundsUsed  + gNumCheckPointBoundsUsed) * gDelta);
   DebugMessage(gFinishedByTime, 1, "   ****** Time Expired ******\n");
   DebugMessage(1, 1, "   allocation %ld\n", MGetTotalAllocation());
 
   return 0;     
}
