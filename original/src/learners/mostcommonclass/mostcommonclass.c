#include "vfml.h"
#include <stdio.h>
#include <string.h>

#include <sys/times.h>
#include <time.h>

char *gFileStem = "DF";
char *gSourceDirectory = ".";
int   gDoTests = 0;
int   gMessageLevel = 0;

static void _printUsage(char  *name) {
   printf("%s: A 'learner' which predicts the most common class in the\n",
                                                                        name);
   printf("   training set.\n");

   printf("-f <filestem>\tSet the name of the dataset (default DF)\n");
   printf("-source <dir>\tSet the source data directory (default '.')\n");
   printf("-u\t\tTest the learner's accuracy on data in <stem>.test\n");

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
      } else if(!strcmp(argv[i], "-source")) {
         gSourceDirectory = argv[i+1];
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-u")) {
         gDoTests = 1;
      } else if(!strcmp(argv[i], "-v")) {
         gMessageLevel++;
         DebugSetMessageLevel(gMessageLevel);
      } else if(!strcmp(argv[i], "-h")) {
         _printUsage(argv[0]);
         exit(0);
      } else {
         printf("Unknown argument: %s.  use -h for help\n", argv[i]);
         exit(0);
      }
   }

   if(gMessageLevel >= 1) {
      printf("Stem: %s\n", gFileStem);
      printf("Source: %s\n", gSourceDirectory);
      if(gDoTests) {
         printf("Running tests\n");
      }
   }
}

int main(int argc, char *argv[]) {
   char fileNames[255];

   FILE *exampleIn;
   ExampleSpecPtr es;
   ExamplePtr e;

   long *classCounts;
   long seen;
   int i;
   int mostCommonClass;

   long tested, errors;

   struct tms starttime;
   struct tms endtime;

   _processArgs(argc, argv);

   sprintf(fileNames, "%s/%s.names", gSourceDirectory, gFileStem);
   es = ExampleSpecRead(fileNames);
   DebugError(es == 0, "Unable to open the .names file");

   /* initialize the counts */
   classCounts = MNewPtr(sizeof(long) * ExampleSpecGetNumClasses(es));
   for(i = 0 ; i < ExampleSpecGetNumClasses(es) ; i++) {
      classCounts[i] = 0;
   }

   if(gMessageLevel >= 1) {
      printf("allocation %ld\n", MGetTotalAllocation());
   }

   sprintf(fileNames, "%s/%s.data", gSourceDirectory, gFileStem);
   exampleIn = fopen(fileNames, "r");
   DebugError(exampleIn == 0, "Unable to open the data file");

   seen = 0;
   times(&starttime);
   e = ExampleRead(exampleIn, es);
   while(e != 0) {
      if(!ExampleIsClassUnknown(e)) {
         seen++;
         classCounts[ExampleGetClass(e)]++;
      }

      ExampleFree(e);
      e = ExampleRead(exampleIn, es);
   }
   fclose(exampleIn);

   if(gMessageLevel >= 1) {
      printf("done scanning...\n");

      printf("   allocation %ld\n", MGetTotalAllocation());

      times(&endtime);
      printf("time %.2lfs\n", ((double)(endtime.tms_utime) -
                       (double)(starttime.tms_utime)) / 100);
   }

   /* find the most common class */
   mostCommonClass = 0;
   for(i = 0 ; i < ExampleSpecGetNumClasses(es) ; i++) {
      if(classCounts[i] > classCounts[mostCommonClass]) {
         mostCommonClass = i;
      }
   }

   if(gDoTests) {
      tested = 0;
      errors = 0;

      sprintf(fileNames, "%s/%s.test", gSourceDirectory, gFileStem);
      exampleIn = fopen(fileNames, "r");
      DebugError(exampleIn == 0, "Unable to open the .test file");
      
      if(gMessageLevel >= 1) {
         printf("opened test file, starting scan...\n");
      }

      times(&starttime);
      e = ExampleRead(exampleIn, es);
      while(e != 0) {
         if(!ExampleIsClassUnknown(e)) {
            tested++;
            if(ExampleGetClass(e) != mostCommonClass) {
               errors++;
            }
         }
         ExampleFree(e);
         e = ExampleRead(exampleIn, es);
      }
      fclose(exampleIn);

      printf("%f\t0\n", ((float)errors/(float)tested) * 100);
   } 
   
   if(gMessageLevel > 0) {
      printf("The most common class is %s.\n",
               ExampleSpecGetClassValueName(es, mostCommonClass));

      if(gMessageLevel >= 1) {
         printf("Seen %ld examples\n", seen);
         for(i = 0 ; i < ExampleSpecGetNumClasses(es) ; i++) {
            printf("%s : %.6f (%ld)\n", ExampleSpecGetClassValueName(es, i),
                   (float)classCounts[i] / (float)seen, classCounts[i]);
         }
      }
   }

   if(gMessageLevel >= 1) {
      printf("allocation %ld\n", MGetTotalAllocation());

      times(&endtime);
      printf("time %.2lfs\n", ((double)(endtime.tms_utime) -
                       (double)(starttime.tms_utime)) / 100);
   }

   return 0;
}
