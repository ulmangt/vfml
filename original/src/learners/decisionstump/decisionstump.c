#include "vfml.h"
#include <stdio.h>
#include <string.h>

#include <sys/times.h>
#include <time.h>

char *gFileStem = "DF";
char *gSourceDirectory = ".";
int   gDoTests = 0;
int   gOutputStump = 0;
int   gOutputGains = 0;
int   gMessageLevel = 0;
int   gMaxThresholds = -1;
int   gPrintAttribute = 0;

static void _printUsage(char  *name) {
   printf("%s - learn a decision tree stump\n", name);
   printf("   takes input in c4.5 format (.names/.data/.test)\n");
   printf("-source <dir> \tSet the source data directory (default '.')\n");
   printf("-f <filestem> \t (default DF)\n");
   printf("-maxThresholds <num> Use the first num values from the training set as\n\t\t thresholds for continuous attributes, allows this program to\n\t\t be run on very large data sets (default use all values)\n");
   printf("-a\t\t print the name of the attribute selected for the\n\t\t split (defaults to not print)\n");
   printf("-u     \t\t test on the data in stem.test (default off)\n");
   printf("-outputStump \t Defaults to output without the -u flag and not output with it\n");
   printf("-outputGains \t Outputs the information gain of each feature\n");
   printf("-v \t\t use up to four times to increase diagnostic output\n");
}

static void _processArgs(int argc, char *argv[]) {
   int i;

   /* HERE on the ones that use the next arg make sure it is there */
   for(i = 1 ; i < argc ; i++) {
      if(!strcmp(argv[i], "-f")) {
         gFileStem = argv[i+1];
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-u")) {
         gDoTests = 1;
      } else if(!strcmp(argv[i], "-outputStump")) {
         gOutputStump = 1;
      } else if(!strcmp(argv[i], "-outputGains")) {
         gOutputGains = 1;
      } else if(!strcmp(argv[i], "-source")) {
         gSourceDirectory = argv[i+1];
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-v")) {
         gMessageLevel++;
         DebugSetMessageLevel(gMessageLevel);
      } else if(!strcmp(argv[i], "-maxThresholds")) {
         sscanf(argv[i+1], "%d", &gMaxThresholds);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-a")) {
         gPrintAttribute = 1;
      } else if(!strcmp(argv[i], "-h")) {
         _printUsage(argv[0]);
         exit(0);
      } else {
         printf("Unknown argument: %s.  use -h for help\n", argv[i]);
         exit(0);
      }
   }

   if(gMessageLevel >= 1) {
      printf("Running with arguments: \n");
      printf("   file stem: %s\n", gFileStem);
      if(gOutputStump) {
         printf("   Output the learned stump\n");
      }
   }
}

int main(int argc, char *argv[]) {
   char fileNames[255];

   FILE *exampleIn;
   ExampleSpecPtr es;
   ExamplePtr e;
   DecisionTreePtr dt;
   ExampleGroupStatsPtr egs;

   long tries, mistakes;
   int class, i, best, count;
   float bestVal, index, threshold, totalEntropy;
   float firstContThresh, firstContIndex, secondContThresh, secondContIndex;

   struct tms starttime;

   _processArgs(argc, argv);

   sprintf(fileNames, "%s.names", gFileStem);
   /* TODO check that the file exists */
   es = ExampleSpecRead(fileNames);

   egs = ExampleGroupStatsNew(es, AttributeTrackerInitial(es));

   if(gMessageLevel >= 1) {
      printf("allocation %ld\n", MGetTotalAllocation());
   }

   sprintf(fileNames, "%s.data", gFileStem);
   /* TODO check that the file exists */
   exampleIn = fopen(fileNames, "r");

   count = 0;
   e = ExampleRead(exampleIn, es);
   while(e != 0) {
      /* DO THE LEARNING! */
      count++;

      ExampleGroupStatsAddExample(egs, e);

      for(i = 0 ; i < ExampleSpecGetNumAttributes(es) ; i++) {
         if(ExampleSpecIsAttributeContinuous(es, i)) {
            if(ExampleGroupStatsNumSplitThresholds(egs, i) == gMaxThresholds) {
               ExampleGroupStatsStopAddingSplits(egs, i);
            }
         }
      }

      ExampleFree(e);
      e = ExampleRead(exampleIn, es);
   }
   fclose(exampleIn);

   dt = DecisionTreeNew(es);

   totalEntropy = ExampleGroupStatsEntropyTotal(egs);
   DebugMessage(1, 1, "Entropy of collection is: %f\n", totalEntropy);

   /* find best attribute for split */
   best = 0;
   threshold = 0;
   bestVal = 1000;
   for(i = 0 ; i < ExampleSpecGetNumAttributes(es) ; i++) {
      if(!ExampleSpecIsAttributeIgnored(es, i)) {
         if(ExampleSpecIsAttributeDiscrete(es, i)) {
            index = ExampleGroupStatsEntropyDiscreteAttributeSplit(egs, i);
            if(index < bestVal) {
               bestVal = index;
               best = i;
            }
            if(gOutputGains) {
               printf("%12s\t\t score: %f\t gain: %f\n", ExampleSpecGetAttributeName(es, i), index, totalEntropy - index);
            }
         } else { /* must be continuous */
            ExampleGroupStatsEntropyContinuousAttributeSplit(egs, i,
                  &firstContIndex, &firstContThresh, &secondContIndex,
                                              &secondContThresh);
            if(firstContIndex < bestVal) {
               bestVal = firstContIndex;
               threshold = firstContThresh;
               best = i;
            }
            if(gOutputGains) {
               printf("%12s - %.2f\t score: %f\t gain: %f\n", ExampleSpecGetAttributeName(es, i), firstContThresh, firstContIndex, totalEntropy - firstContIndex);
            }
         }
      }
   }

   if(ExampleSpecIsAttributeDiscrete(es, best)) {
      DebugMessage(1, 2, "Best attribute is %d with score %f\n", 
                                                      best, bestVal);
      DecisionTreeSplitOnDiscreteAttribute(dt, best);

      for(i = 0 ; i < DecisionTreeGetChildCount(dt) ; i++) {
         DecisionTreeSetClass(DecisionTreeGetChild(dt, i), 
              ExampleGroupStatsGetMostCommonClassForAttVal(egs, best, i));
         DecisionTreeSetTypeLeaf(DecisionTreeGetChild(dt, i));
      }
   } else { /* must be continuous */
      DebugMessage(1, 2, "Best attribute is %d threshold %f with score %f\n", 
                                                best, threshold, bestVal);
      DecisionTreeSplitOnContinuousAttribute(dt, best, threshold);

      DecisionTreeSetClass(DecisionTreeGetChild(dt, 0), 
           ExampleGroupStatsGetMostCommonClassBelowThreshold(egs, 
                                                       best, threshold));
      DecisionTreeSetTypeLeaf(DecisionTreeGetChild(dt, 0));

      DecisionTreeSetClass(DecisionTreeGetChild(dt, 1), 
           ExampleGroupStatsGetMostCommonClassAboveThreshold(egs, 
                                                       best, threshold));
      DecisionTreeSetTypeLeaf(DecisionTreeGetChild(dt, 1));
   }

   if(gMessageLevel >= 1) {
      printf("done learning...\n");
      DecisionTreeWrite(dt, stdout);

      printf("   allocation %ld\n", MGetTotalAllocation());

      times(&starttime);
      printf("utime %.2lfs\n", (double)(starttime.tms_utime) / 100);
      printf("stime %.2lfs\n", (double)(starttime.tms_stime) / 100);
   }

   if((!gDoTests || gOutputStump) && !gPrintAttribute) {
      DecisionTreePrint(dt, stdout);
   }

   if(gPrintAttribute) {
      printf("%s\n", ExampleSpecGetAttributeName(es, best));
   }

   if(gDoTests) {
      tries = 0;
      mistakes = 0;

      sprintf(fileNames, "%s.test", gFileStem);
      /* TODO check that the file exists */
      exampleIn = fopen(fileNames, "r");

      if(gMessageLevel >= 1) {
         printf("opened test file, starting tests\n");
      }

      e = ExampleRead(exampleIn, es);
      while(e != 0) {
         tries++;
         class = DecisionTreeClassify(dt, e);
         if(ExampleGetClass(e) != class) {
            mistakes++;
         }
         ExampleFree(e);
         e = ExampleRead(exampleIn, es);
      }
      fclose(exampleIn);

      printf("%f %d\n", (((float)mistakes/(float)tries)) * 100,
                   DecisionTreeCountNodes(dt));
   }

   if(gMessageLevel >= 1) {
      printf("allocation %ld\n", MGetTotalAllocation());

      times(&starttime);
      printf("utime %.2lfs\n", (double)(starttime.tms_utime) / 100);
      printf("stime %.2lfs\n", (double)(starttime.tms_stime) / 100);
   }

   return 0;
}
