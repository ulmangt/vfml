#include "vfml.h"
#include <stdio.h>

//#include <string.h>

char  *gFileStem = "DF";
int   gFoldCount = 10;
int   gMessageLevel = 0;
char  *gTargetDirectory = ".";
char  *gSourceDirectory = ".";
long  gSeed = -1;
int   gError = 0;

static void _printUsage(char  *name) {
   printf("%s: splits a dataset into a collection of training and testing\n",
                                                                        name);
   printf("   sets for use with cross validation.  Writes the new datasets\n");
   printf("   as <stem>[0-n].<tail>\n");

   printf("-f <filestem>\tSet the name of the dataset (default DF)\n");
   printf("-target <dir>\tSet the output directory (default '.')\n");
   printf("-source <dir>\tSet the source data directory (default '.')\n");
   printf("-folds <n>\tSets the number of train/test sets to create (default 10)\n");
   printf("-seed <n>\tSets the random seed, multiple runs with the same seed will\n");
   printf("\t\t   produce the same datasets (defaults to a random seed)\n");

   printf("-v\t\tCan be used multiple times to increase the debugging output\n");
}

static void _processArgs(int argc, char *argv[]) {
   int i;

   /* HERE on the ones that use the next arg make sure it is there */
   for(i = 1 ; i < argc ; i++) {
      if(!strcmp(argv[i], "-f")) {
         gFileStem = argv[i+1];
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-target")) {
         gTargetDirectory = argv[i+1];
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-source")) {
         gSourceDirectory = argv[i+1];
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-folds")) {
         sscanf(argv[i+1], "%d", &gFoldCount);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-v")) {
         gMessageLevel++;
      } else if(!strcmp(argv[i], "-seed")) {
         sscanf(argv[i+1], "%ld", &gSeed);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-h")) {
         _printUsage(argv[0]);
         exit(0);
      } else {
         printf("Unknown argument: %s.  use -h for help\n", argv[i]);
         exit(0);
      }
   }

   if(gMessageLevel >= 1) {
      printf("File stem: %s\n", gFileStem);
      printf("Target: %s\n", gTargetDirectory);
      printf("Source: %s\n", gSourceDirectory);
      printf("Folds: %d\n", gFoldCount);
      printf("Seed: %ld\n", gSeed);
   }
}

int main(int argc, char *argv[]) {
   char fileName[255];
   int i, targetFold;
   FILE *exampleIn, *names;
   ExampleSpecPtr es;
   ExamplePtr e;
   FILE **outputData;
   FILE **outputTest;
   long *outputCounts;

   _processArgs(argc, argv);

   /* set up the random seed */
   RandomInit();
   if(gSeed != -1) {
      RandomSeed(gSeed);
   }

   /* read the spec file */
   sprintf(fileName, "%s/%s.names", gSourceDirectory, gFileStem);
   /* TODO check that the file exists */
   es = ExampleSpecRead(fileName);
   DebugError(es == 0, "Unable to open the .names file");


   /* set up the output files */
   outputData = MNewPtr(sizeof(FILE *) * gFoldCount);
   outputTest = MNewPtr(sizeof(FILE *) * gFoldCount);
   outputCounts = MNewPtr(sizeof(long) * gFoldCount);

   for(i = 0 ; i < gFoldCount ; i++) {
      /* TODO check that the target directory exists */
      sprintf(fileName, "%s/%s%d.data", gTargetDirectory, gFileStem, i);
      outputData[i] = fopen(fileName, "w");
      sprintf(fileName, "%s/%s%d.test", gTargetDirectory, gFileStem, i);
      outputTest[i] = fopen(fileName, "w");
      outputCounts[i] = 0;

      DebugError(outputData[i] == 0 || outputTest[i] == 0, "Unable to open output files.");
   }

   sprintf(fileName, "%s/%s.data", gSourceDirectory, gFileStem);
   exampleIn = fopen(fileName, "r");
   DebugError(exampleIn == 0, "Unable to open .data file");

   e = ExampleRead(exampleIn, es);
   while(e != 0) {
      /* DO THE FOLDING */
      targetFold = RandomRange(0, gFoldCount - 1);
      for(i = 0 ; i < gFoldCount ; i++) {
         if(i == targetFold) {
            ExampleWrite(e, outputTest[i]);
            outputCounts[i]++;
         } else {
            ExampleWrite(e, outputData[i]);
	 }
      }

      ExampleFree(e);
      e = ExampleRead(exampleIn, es);
   }
   fclose(exampleIn);

   /* close the output files */
   for(i = 0 ; i < gFoldCount ; i++) {
      /* TODO check that the target directory exists */
      fclose(outputData[i]);
      fclose(outputTest[i]);
      if(gMessageLevel >= 1) {
         printf("fold %d: %ld\n", i, outputCounts[i]);
      }
   }

   /* output the names files */
   for(i = 0 ; i < gFoldCount ; i++) {
      /* TODO check that the target directory exists */
      sprintf(fileName, "%s/%s%d.names", gTargetDirectory, gFileStem, i);
      names = fopen(fileName, "w");
      DebugError(names == 0, "Unable to open target names file for output.");

      ExampleSpecWrite(es, names);
      fclose(names);
   }

   return 0;
}

