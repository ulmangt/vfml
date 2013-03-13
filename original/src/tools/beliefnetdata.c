#include "vfml.h"
#include <stdio.h>
#include <string.h>
#include <sys/times.h>
#include <time.h>
#include <math.h>

char  *gFileStem         = "DF";
char  *gBeliefNetFile    = "DF.bif";
int   gInfiniteStream    = 0;
char  *gTargetDirectory  = ".";
unsigned long  gNumTrainExamples = 10000;
float gErrorLevel = 0.00;


long  gSeed = -1;
int   gStdout = 0;

//static char *_AllocateString(char *first) {
//   char *second = MNewPtr(sizeof(char) * (strlen(first) + 1));
//
//   strcpy(second, first);
//
//   return second;
//}

static void _processArgs(int argc, char *argv[]) {
   int i;

   /* HERE on the ones that use the next arg make sure it is there */
   for(i = 1 ; i < argc ; i++) {
      if(!strcmp(argv[i], "-f")) {
         gFileStem = argv[i+1];
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-bnf")) {
         gBeliefNetFile = argv[i+1];
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-h")) {
         printf("Generate a synthetic dataset\n");
         printf("   -f <stem name for output> (default DF)\n");
         printf("   -bnf <file containing belief net> (default DF.bif)\n");
         printf("   -train <size of training set> (default 10000)\n");
         printf("   -infinite\t Generate an infinite stream of training examples,\n\t overrides -train flags, only makes sense with -stdout (default off)\n");
         printf("   -stdout  output the trainset to stdout (default to <stem>.data)\n");
         printf("   -seed <random seed> (default to random)\n");
         printf("   -noise <noise level> 10 means change 10%% of values, including \n\t classes (default to 0)\n");
         printf("   -target <dir> Set the output directory (default '.')\n");
         printf("   -v increase the message level\n");
         exit(0);
      } else if(!strcmp(argv[i], "-train")) {
         sscanf(argv[i+1], "%lu", &gNumTrainExamples);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-seed")) {
         sscanf(argv[i+1], "%lu", &gSeed);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-noise")) {
         sscanf(argv[i+1], "%f", &gErrorLevel);
         gErrorLevel /= 100.0;
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-v")) {
         DebugSetMessageLevel(DebugGetMessageLevel() + 1);
      } else if(!strcmp(argv[i], "-stdout")) {
         gStdout = 1;
      } else if(!strcmp(argv[i], "-infinite")) {
         gInfiniteStream = 1;
      } else if(!strcmp(argv[i], "-target")) {
         gTargetDirectory = argv[i+1];
         /* ignore the next argument */
         i++;
      } else {
         printf("unknown argument '%s', use -h for help\n", argv[i]);
         exit(0);
      }
   }
}


static BeliefNet _readBN(void) {
   char fileName[255];

   sprintf(fileName, "%s", gBeliefNetFile);

   return BNReadBIF(fileName);
}

static void _writeSpec(BeliefNet bn) {
   char fileName[255];
   FILE *fileOut;

   sprintf(fileName, "%s/%s.names", gTargetDirectory, gFileStem);
   fileOut = fopen(fileName, "w");
   ExampleSpecWrite(BNGetExampleSpec(bn), fileOut);
   //BNWriteBIF(bn, fileOut);

   /* dunno if this flush helps, but sometimes the learner tries
       to open this before the file appears when piping the output
       to the learner via stdout.  Maybe delay a second after this 
       close? */

   fflush(fileOut);
   fclose(fileOut);
}

static void _makeData(BeliefNet bn) {
   char fileNames[255];
   FILE *fileOut;
   ExamplePtr e;
   long i;

   if(!gStdout) {
      sprintf(fileNames, "%s/%s.data", gTargetDirectory, gFileStem);
      fileOut = fopen(fileNames, "w");
   } else {
      fileOut = stdout;
   }

   for(i = 0 ; i < gNumTrainExamples || gInfiniteStream ; i++) {
      e = BNGenerateSample(bn);
      ExampleAddNoise(e, gErrorLevel, 1, 1);
      ExampleWrite(e, fileOut);
      ExampleFree(e);
   }

   if(!gStdout) {
      fclose(fileOut);
   }
}

int main(int argc, char *argv[]) {
   BeliefNet bn;
   _processArgs(argc, argv);

   /* randomize the random # generator */
   RandomInit();
   /* seed for the data */
   if(gSeed != -1) {
      RandomSeed(gSeed);
   } else {
      gSeed = RandomRange(1, 30000);
      RandomSeed(gSeed);
   }

   bn = _readBN();

   DebugError(bn == 0, "Unable to read .bif file.\n");

   if(DebugGetMessageLevel() > 0) {
      BNPrintStats(bn);
   }

   _writeSpec(bn);
   _makeData(bn);

   return 0;
}
