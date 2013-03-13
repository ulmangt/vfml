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
   printf("%s: An implementation of naive bayes which works only with discrete attributes.\n", name);

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

   ExampleGroupStatsPtr egs;
   AttributeTrackerPtr at;

   int i, j;

   double *classEstimates;
   int max;

   long tested, errors;

   struct tms starttime;
   struct tms endtime;

   _processArgs(argc, argv);

   sprintf(fileNames, "%s/%s.names", gSourceDirectory, gFileStem);
   es = ExampleSpecRead(fileNames);
   DebugError(es == 0, "Unable to open the .names file");

   at = AttributeTrackerInitial(es);
   for(i = 0 ; i < ExampleSpecGetNumAttributes(es) ; i++) {
      if(!ExampleSpecIsAttributeDiscrete(es, i)) {
         AttributeTrackerMarkInactive(at, i);
      }
   }

   egs = ExampleGroupStatsNew(es, at);

   if(gMessageLevel >= 1) {
      printf("allocation %ld\n", MGetTotalAllocation());
   }

   sprintf(fileNames, "%s/%s.data", gSourceDirectory, gFileStem);
   exampleIn = fopen(fileNames, "r");
   DebugError(exampleIn == 0, "Unable to open the data file");

   times(&starttime);
   e = ExampleRead(exampleIn, es);
   while(e != 0) {
      if(!ExampleIsClassUnknown(e)) {
         ExampleGroupStatsAddExample(egs, e);
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


   if(gDoTests) {
      classEstimates = MNewPtr(sizeof(double) * ExampleSpecGetNumClasses(es));
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
            if(gMessageLevel >=2 ) {
               ExampleWrite(e, stdout);
            }

            tested++;
            for(i = 0 ; i < ExampleSpecGetNumClasses(es) ; i++) {
               classEstimates[i] = ExampleGroupStatsGetClassLogP(egs, i);
               if(gMessageLevel >= 3 ) {
                  printf("lp(%s) = %lf\n", ExampleSpecGetClassValueName(es, i),
                          classEstimates[i]);
               }
               for(j = 0 ; j < ExampleSpecGetNumAttributes(es) ; j++) {
                  if(AttributeTrackerIsActive(at, j) &&
                                      !ExampleIsAttributeUnknown(e, j)) {
                     classEstimates[i] += 
                      ExampleGroupStatsGetValueGivenClassMEstimateLogP(
                           egs, j, ExampleGetDiscreteAttributeValue(e, j), i);

                     if(gMessageLevel >= 3 ) {
                        printf("   p(%s|%s) = %lf product %.100lf\n",
                          ExampleSpecGetAttributeValueName(es, j,
                               ExampleGetDiscreteAttributeValue(e, j)),
                          ExampleSpecGetClassValueName(es, i),
                          ExampleGroupStatsGetValueGivenClassMEstimateLogP(
                           egs, j, ExampleGetDiscreteAttributeValue(e, j), i),
                           classEstimates[i]);

                     }
                  }
               }
            }
            max = 0;
            for(i = 1 ; i < ExampleSpecGetNumClasses(es) ; i++) {
               if(classEstimates[i] > classEstimates[max]) {
                  max = i;
               }
            }

            if(gMessageLevel >=2 ) {
               printf("%s: ", ExampleSpecGetClassValueName(es, max));
               for(i = 0 ; i < ExampleSpecGetNumClasses(es) ; i++) {
                  printf("%lf ", classEstimates[i]);
               }
               printf("\n");
            }

            if(ExampleGetClass(e) != max) {
               errors++;
            }
         }
         ExampleFree(e);
         e = ExampleRead(exampleIn, es);
      }
      fclose(exampleIn);

      printf("%f\t0\n", ((1.0 - (float)errors/(float)tested)) * 100);
   } else {
      ExampleGroupStatsWrite(egs, stdout);
   }

   if(gMessageLevel >= 1) {
      printf("allocation %ld\n", MGetTotalAllocation());

      times(&endtime);
      printf("time %.2lfs\n", ((double)(endtime.tms_utime) -
                       (double)(starttime.tms_utime)) / 100);
   }

   return 0;
}
