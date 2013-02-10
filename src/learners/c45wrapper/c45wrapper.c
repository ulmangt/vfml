#include "vfml.h"
#include <stdio.h>
#include <string.h>

#include <sys/times.h>
#include <time.h>

char *gFileStem = "DF";
char *gSourceDirectory = ".";
char  *gTmpFileName = "c4.5wraper-tmp-file.out";
char  *gArgs = "";
char  *gTest = "-u";
int   gMessageLevel = 0;
int   gPrune     = 1;

static void _printUsage(char  *name) {
   printf("%s: Invokes C4.5 on the dataset & outputs error/size information in UWML format (error <white space> size <white space> time)\n", name);

   printf("-f <filestem>\t Set the name of the dataset (default DF)\n");
   printf("-args <string>\t Pass <string> as additional arguments to C4.5\n");
   printf("-noPrune \t Reports the results from c4.5's unpurned tree (default\n\t\t reports from pruned tree)\n");
   printf("-noTest \t Suppress the -u argument which will make c4.5\n\t\t output accuracy on the training set (default test\n\t\t accuracy on the <stem>.test file)\n");
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
      } else if(!strcmp(argv[i], "-args")) {
         gArgs = argv[i+1];
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-v")) {
         gMessageLevel++;
      } else if(!strcmp(argv[i], "-noPrune")) {
         gPrune = 0;
      } else if(!strcmp(argv[i], "-noTest")) {
         gTest = "";
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
   }
}

int main(int argc, char *argv[]) {
   char command[255];
   FILE *results;
   int matched, size, nopruneSize;
   float error, nopruneError;
   int done;

   struct tms starttime;
   struct tms endtime;

   _processArgs(argc, argv);

   sprintf(command, "c4.5 %s %s -f %s | grep \"<<\" >> %s", gArgs, gTest, gFileStem, gTmpFileName);

   if(gMessageLevel >= 1) {
      printf("%s\n", command);
   }

   times(&starttime);
   system(command);
   times(&endtime);

   if(gMessageLevel >= 1) {
      printf("time %.2lfs\n", ((double)(endtime.tms_utime) -
                       (double)(starttime.tms_utime)) / 100);
   }

   results = fopen(gTmpFileName, "r");
   done = 0;
   while(!done) {
      matched = fscanf(results, " %d %*d ( %f %% ) %d %*d ( %f %% ) ( %*f %%) << ",
                           &nopruneSize, &nopruneError, &size, &error);
      if(gMessageLevel >= 1) {
         printf("matched %d\n", matched);
      }

      if(matched < 1) {
         done = 1;
      }
   }
   
   if(gPrune) {
      printf("%f\t%d\t%.2f\n", error, size, ((double)(endtime.tms_cutime) -
                       (double)(starttime.tms_cutime)) / 100);
   } else {
      printf("%f\t%d\t%.2f\n", nopruneError, nopruneSize, 
                       ((double)(endtime.tms_cutime) -
                            (double)(starttime.tms_cutime)) / 100);
   }

   sprintf(command, "rm %s %s.unpruned %s.tree", gTmpFileName,
                                                  gFileStem, gFileStem);
   if(gMessageLevel >= 1) {
      printf("%s\n", command);
   }
   system(command);

   return 0;
}

