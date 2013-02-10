#include "vfml.h"
#include <stdio.h>

//#include <string.h>

char  *gFileStem = "DF";
char  *gOutputStem = "DF-out";
float gSamplePercent = .8;
int   gMessageLevel = 0;
char  *gTargetDirectory = ".";
char  *gSourceDirectory = ".";
int   gStdin = 0;
int   gStdout = 0;
int   gOutputTest = 0;
long  gSeed = -1;

static void _printUsage(char  *name) {
   printf("%s: splits a dataset into a training and testing set\n", name);

   printf("-f <filestem>\tSet the name of the dataset (default DF)\n");
   printf("-fout <filestem>\tSet the name of the output dataset (default DF-out)\n");
   printf("-target <dir>\tSet the output directory (default '.')\n");
   printf("-source <dir>\tSet the source data directory (default '.')\n");
   printf("-sample <n>\tSets the percentage of the data to sample\n");
   printf("\t\t(default .8 -- that's 80%%)\n");
   printf("-stdin \tRead examples from standard in (default read file)\n");
   printf("-stdout \tWrite sample to standard out (default write file)\n");
   printf("-outputTest \tWrite remaining data to <stem>.test (default do not)\n");
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
      } else if(!strcmp(argv[i], "-fout")) {
         gOutputStem = argv[i+1];
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
      } else if(!strcmp(argv[i], "-sample")) {
         sscanf(argv[i+1], "%f", &gSamplePercent);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-v")) {
         gMessageLevel++;
      } else if(!strcmp(argv[i], "-stdin")) {
         gStdin = 1;
      } else if(!strcmp(argv[i], "-stdout")) {
         gStdout = 1;
      } else if(!strcmp(argv[i], "-outputTest")) {
         gOutputTest = 1;
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
      printf("Sample Percentage: %.5f\n", gSamplePercent);
      printf("Seed: %ld\n", gSeed);
   }
}

int main(int argc, char *argv[]) {
   char fileName[255];
   FILE *exampleIn, *names;
   ExampleSpecPtr es;
   ExamplePtr e;
   FILE *outputData, *outputTest = NULL;

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

   if(!gStdout) {
      /* TODO check that the target directory exists */
      sprintf(fileName, "%s/%s.data", gTargetDirectory, gOutputStem);
      outputData = fopen(fileName, "w");
      DebugError(outputData == 0, "Unable to open output files.");
   } else {
      outputData = stdout;
   }

   if(gOutputTest) {
      /* TODO check that the target directory exists */
      sprintf(fileName, "%s/%s.test", gTargetDirectory, gOutputStem);
      outputTest = fopen(fileName, "w");
      DebugError(outputData == 0, "Unable to open test data output files.");
   }

   if(!gStdin) {
      sprintf(fileName, "%s/%s.data", gSourceDirectory, gFileStem);
      exampleIn = fopen(fileName, "r");
      DebugError(exampleIn == 0, "Unable to open .data file");
   } else {
      exampleIn = stdin;
   }

   e = ExampleRead(exampleIn, es);
   while(e != 0) {
      if(gSamplePercent >= RandomDouble()) {
         ExampleWrite(e, outputData);
      } else if(gOutputTest) {
         ExampleWrite(e, outputTest);
      }

      ExampleFree(e);
      e = ExampleRead(exampleIn, es);
   }
   fclose(exampleIn);


   fclose(outputData);

   /* output the names file */
   /* TODO check that the target directory exists */
   sprintf(fileName, "%s/%s.names", gTargetDirectory, gOutputStem);
   names = fopen(fileName, "w");
   DebugError(names == 0, "Unable to open target names file for output.");

   ExampleSpecWrite(es, names);
   fclose(names);

   return 0;
}

