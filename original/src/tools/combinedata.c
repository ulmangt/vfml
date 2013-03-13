#include "vfml.h"
#include <stdio.h>

//#include <string.h>

char  *gNamesFile = "DF.names";
char  *gOutputStem = "DF-out";
int   gFirstFileIndex = -1;
int   gMessageLevel = 0;
char  *gTargetDirectory = ".";
char  *gSourceDirectory = ".";
long  gSeed = -1;

static void _printUsage(char  *name) {
   printf("%s: combines a bunch of data files into a single big file\n", name);

   printf("-names <filename>\tSet the names file (default DF.names)\n");
   printf("-fout <filestem>\tSet the name of the output dataset (default DF-out)\n");
   printf("-target <dir>\tSet the output directory (default '.')\n");
   printf("-source <dir>\tSet the source data directory (default '.')\n");
   printf("-dataFiles <list of files>\t till the end of the line list the files to combine (REQUIRED)\n");

   printf("-v\t\tCan be used multiple times to increase the debugging output\n");
}

static void _processArgs(int argc, char *argv[]) {
   int i, done = 0;

   /* HERE on the ones that use the next arg make sure it is there */
   for(i = 1 ; i < argc && !done; i++) {
      if(!strcmp(argv[i], "-names")) {
         gNamesFile = argv[i+1];
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
      } else if(!strcmp(argv[i], "-dataFiles")) {
         done = 1;
         gFirstFileIndex = i + 1;
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

   DebugError(gFirstFileIndex == -1, "Missing required -dataFiles arg");

   if(gMessageLevel >= 1) {
      printf("Combining %d files\n", argc - gFirstFileIndex);
      printf("Names File: %s\n", gNamesFile);
      printf("Target: %s\n", gTargetDirectory);
      printf("Source: %s\n", gSourceDirectory);
      printf("Seed: %ld\n", gSeed);
   }
}

int main(int argc, char *argv[]) {
   char fileName[255];
   int i;
   FILE *exampleIn, *names;
   ExampleSpecPtr es;
   ExamplePtr e;
   FILE *outputData;

   _processArgs(argc, argv);

   /* set up the random seed */
   RandomInit();
   if(gSeed != -1) {
      RandomSeed(gSeed);
   }

   /* read the spec file */
   sprintf(fileName, "%s/%s", gSourceDirectory, gNamesFile);
   /* TODO check that the file exists */
   es = ExampleSpecRead(fileName);
   DebugError(es == 0, "Unable to open the .names file");

   /* TODO check that the target directory exists */
   sprintf(fileName, "%s/%s.data", gTargetDirectory, gOutputStem);
   outputData = fopen(fileName, "w");

   DebugError(outputData == 0, "Unable to open output files.");

   for(i = gFirstFileIndex ; i < argc ; i++) {
      DebugMessage(1, 1, "%s...\n", argv[i]);

      sprintf(fileName, "%s/%s", gSourceDirectory, argv[i]);
      exampleIn = fopen(fileName, "r");
      DebugError(exampleIn == 0, "Unable to open .data file");

      e = ExampleRead(exampleIn, es);
      while(e != 0) {
         ExampleWrite(e, outputData);
         ExampleFree(e);
         e = ExampleRead(exampleIn, es);
      }
      fclose(exampleIn);
   }

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

