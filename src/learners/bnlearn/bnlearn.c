#include "vfml.h"
#include <stdio.h>
#include <string.h>

#include <assert.h>

#include <sys/times.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

static void _printUsage(char  *name) {
   printf("%s : Learn the structure (and parameters) of a Belief Net\n", name);

   printf("-f <filestem>\t Set the name of the dataset (default DF)\n");
   printf("-source <dir>\t Set the source data directory (default '.')\n");
   printf("-startFrom <filename>\t use net in <filename> as starting point,\n\t\t must be BIF file (default start from empty net)\n");
   printf("-outputTo <filename>\t output the learned net to <filename>\n");
   printf("-limitMegs <count>\t Limit dynamic memory allocation to 'count'\n\t\t megabytes, don't consider networks that take too much\n\t\t space (default no limit)\n");
   printf("-limitMinutes <count>\t Limit the run to <count> minutes then\n\t\t return current model (default no limit)\n");
   printf("-noCacheTrainSet \t Repeatedly read training data from disk and do not\n\t\t cache it in RAM (default read data into RAM\n\t\t at beginning of run)\n");
   printf("-stdin \t\t Reads training examples from stdin instead of from\n\t\t <stem>.data causes a 2 second delay to help give\n\t\t input time to setup (default off, not compatable\n\t\t with -noCacheTrainSet)\n");
   printf("-noReverse \t Doesn't reverse links to make nets for next search\n\t\t step (default reverse links)\n");
   printf("-parametersOnly\t Only estimate parameters for current\n\t\t structure, no other learning\n");
   printf("-seed <s>\t Seed for random numbers (default random)\n");
   printf("-maxSearchSteps <num>\tLimit to <num> search steps (default no max).\n");
   printf("-maxParentsPerNode <num>\tLimit each node to <num> parents\n\t\t (default no max).\n");
   printf("-maxParameterGrowthMult <mult>\tLimit net to <mult> times starting\n\t\t # of parameters (default no max).\n");
   printf("-maxParameterCount <count>\tLimit net to <count> parameters\n\t\t (default no max).\n");
   printf("-kappa <kappa>  the structure prior penalty for batch (0 - 1), 1 is\n\t\t no penalty (default 0.5)\n");
   printf("-v\t\t Can be used multiple times to increase the debugging output\n");
}


static void _processArgs(int argc, char *argv[], BNLearnParams *params) {
   int i;
   int cacheTrainSet=1;
   char filename[2000];
   ExampleSpecPtr es;
   char *sourceDirectory=".", *fileStem="DF";
   char *inputNetFilename=NULL;
   int useStdin=0;
   FILE *in;

   /* HERE on the ones that use the next arg make sure it is there */
   for(i = 1 ; i < argc ; i++) {
      if(!strcmp(argv[i], "-f")) {
         fileStem = argv[i+1];
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-startFrom")) {
         inputNetFilename = argv[i+1];
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-outputTo")) {
         params->gOutputNetFilename = argv[i+1];
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-source")) {
         sourceDirectory = argv[i+1];
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-v")) {
         DebugSetMessageLevel(DebugGetMessageLevel() + 1);
      } else if(!strcmp(argv[i], "-h")) {
         _printUsage(argv[0]);
         exit(0);
      } else if(!strcmp(argv[i], "-kappa")) {
         sscanf(argv[i+1], "%lf", &params->gKappa);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-limitMegs")) {
         sscanf(argv[i+1], "%ld", &params->gLimitBytes);
         params->gLimitBytes *= 1024 * 1024;
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-limitMinutes")) {
         sscanf(argv[i+1], "%lf", &params->gLimitSeconds);
         params->gLimitSeconds *= 60;
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-maxSearchSteps")) {
         sscanf(argv[i+1], "%d", &params->gMaxSearchSteps);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-maxParentsPerNode")) {
         sscanf(argv[i+1], "%d", &params->gMaxParentsPerNode);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-maxParameterGrowthMult")) {
         sscanf(argv[i+1], "%d", &params->gMaxParameterGrowthMult);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-maxParameterCount")) {
         sscanf(argv[i+1], "%ld", &params->gMaxParameterCount);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-noCacheTrainSet")) {
         cacheTrainSet=0;
      } else if(!strcmp(argv[i], "-stdin")) {
         sleep(2);
         useStdin=1;
      } else if(!strcmp(argv[i], "-parametersOnly")) {
         params->gOnlyEstimateParameters = 1;
      } else if(!strcmp(argv[i], "-noReverse")) {
         params->gNoReverse = 1;
      } else if(!strcmp(argv[i], "-seed")) {
         sscanf(argv[i+1], "%d", &params->gSeed);
         /* ignore the next argument */
         i++;
      } else {
         printf("Unknown argument: %s.  use -h for help\n", argv[i]);
         exit(0);
      }
   }

   // Verify params
   if(useStdin && (cacheTrainSet == 0)) {
      DebugError(1, "Can not use -stdin with -noCacheTrainSet\n");
   }
   // Set up the input data
   sprintf(filename, "%s/%s.names", sourceDirectory, fileStem);
   es = ExampleSpecRead(filename);
   DebugError(es == 0, "Unable to open the .names file");

   sprintf(filename, "%s/%s.data", sourceDirectory, fileStem);
   in = fopen(filename, "r");
   DebugError(in == 0, "Unable to open the .data file");

   if (cacheTrainSet) {
      params->gDataMemory = ExamplesRead(in,es);
      DebugError(!params->gDataMemory, "Unable to read the .data file");
   }
   else {
      params->gDataFile = in;
   }
   DebugMessage(1, 1, "Stem: %s\n", fileStem);
   DebugMessage(1, 1, "Source: %s\n", sourceDirectory);
   DebugMessage(useStdin, 1, "Reading examples from standard in.\n");

   // Set up the input network
   if (inputNetFilename) {
      params->gInputNetFile = fopen(inputNetFilename, "r");
      DebugError(!params->gInputNetFile, "Unable to reading input network");
   }
   else {
      params->gInputNetEmptySpec = es;
   }

}


int main(int argc, char *argv[]) {
   BNLearnParams *params = BNLearn_NewParams();
   params->gOutputNetToMemory = 1;
   _processArgs(argc, argv, params);   

   BNLearn(params);

   printf("Final net:\n");
   BNWriteBIF(params->gOutputNetMemory, stdout);
   BNPrintStats(params->gOutputNetMemory);

   BNLearn_FreeParams(params);
   return 0;
}
