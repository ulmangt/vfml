#include "malloc.h"
#include "vfml.h"
#include <stdio.h>

char *gSourceDirectory = ".";
char *gFileStem = "DF";
char *gTargetFileStem = "shuffled";
char *gTargetDirectory = ".";
long  gSeed = -1;
int   gMessageLevel = 0;

static void _printUsage(char  *name) {
   printf("%s: reads a dataset into memory and outputs it in random order\n",                                           name);
   printf("-f <filestem>\tSet the name of the dataset (default DF)\n");
   printf("-fout <filestem>\tSet the name of the output dataset (default shuffled)\n");
   printf("-target <dir>\tSet the output directory (default '.')\n");
   printf("-source <dir>\tSet the source data directory (default '.')\n");
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
      } else if(!strcmp(argv[i], "-h")) {
         _printUsage(argv[0]);
         exit(0);
      } else if(!strcmp(argv[i], "-source")) {
         gSourceDirectory = argv[i+1];
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-target")) {
         gTargetDirectory = argv[i+1];
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-fout")) {
         gTargetFileStem = argv[i+1];
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-seed")) {
         sscanf(argv[i+1], "%ld", &gSeed);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-v")) {
         gMessageLevel++;
      } else {
         printf("Unknown argument: %s.  use -h for help\n", argv[i]);
         exit(0);
      }
   }

   if(gMessageLevel >= 1) {
      printf("File stem: %s\n", gFileStem);
      printf("Output stem: %s\n", gTargetFileStem);
      printf("Target: %s\n", gTargetDirectory);
      printf("Source: %s\n", gSourceDirectory);
      printf("Seed: %ld\n", gSeed);
   }
}

int main(int argc, char *argv[]) {
   char fileName[255];



   FILE *exampleIn, *names;
   ExampleSpecPtr es;
   ExamplePtr e;
   FILE *outputData;
   VoidAListPtr list;

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

   /* TODO check that the target directory exists */
   sprintf(fileName, "%s/%s.data", gTargetDirectory, gTargetFileStem);
   outputData = fopen(fileName, "w");

   sprintf(fileName, "%s/%s.data", gSourceDirectory, gFileStem);
   /* TODO check that the file exists */
   exampleIn = fopen(fileName, "r");

   list = VALNew();
   e = ExampleRead(exampleIn, es);
   while(e != 0) {
      VALAppend(list, e);
      e = ExampleRead(exampleIn, es);
   }
   fclose(exampleIn);

   if(gMessageLevel >= 1) {
      printf("Done reading, begin outputting...\n");
      fflush(stdout);
   }

   /* now output in random order */
   while(VALLength(list)) {
      e = VALRemove(list, RandomRange(0, VALLength(list) - 1));
      ExampleWrite(e, outputData);

      ExampleFree(e);
   }

   /* close the output file */
   fclose(outputData);

   /* output the names files */
   /* TODO check that the target directory exists */
   sprintf(fileName, "%s/%s.names", gTargetDirectory, gTargetFileStem);
   names = fopen(fileName, "w");
   ExampleSpecWrite(es, names);
   fclose(names);

   return 0;
}
