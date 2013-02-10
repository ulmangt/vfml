#include "vfml.h"
#include <stdio.h>
#include <string.h>

#include <sys/times.h>
#include <time.h>

char *gFileStem = "DF";
char *gSourceDirectory = ".";
char  *gTmpFileName = "c5.0wraper-tmp-file.out";
char  *gArgs = "";
int   gDoTests = 0;
int   gMessageLevel = 0;

static void _printUsage(char  *name) {
   printf("%s: Invokes C5.0 on the dataset & outputs error/size information in UWML format (error <white space> size <white space> time)\n", name);

   printf("-f <filestem>\t Set the name of the dataset (default DF)\n");
   printf("-args <string>\t Pass <string> as additional arguments to C5.0\n");

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
   int matched, size;
   float error;
   int done;

   struct tms starttime;
   struct tms endtime;

   _processArgs(argc, argv);

   sprintf(command, "c5.0 %s -f %s | grep \"<<\" >> %s", gArgs, gFileStem, gTmpFileName);

   if(gMessageLevel >= 1) {
      printf("%s\n", command);
   }

   times(&starttime);
   system(command);
   times(&endtime);

   if(gMessageLevel >= 1) {
      printf("time %.2lfs\n", ((double)(endtime.tms_cutime) -
                       (double)(starttime.tms_cutime)) / 100);
   }

   results = fopen(gTmpFileName, "r");
   done = 0;
   while(!done) {
      matched = fscanf(results, " %d %*d ( %f %% ) <<", &size, &error);
      if(gMessageLevel >= 1) {
         printf("matched %d\n", matched);
      }

      if(matched < 1) {
         done = 1;
      }
   }
   printf("%f\t%d\t%.2lf\n", error, size, ((double)(endtime.tms_cutime) -
                       (double)(starttime.tms_cutime)) / 100);

   sprintf(command, "rm %s %s.tmp %s.tree", gTmpFileName, gFileStem, gFileStem);
   if(gMessageLevel >= 1) {
      printf("%s\n", command);
   }
   system(command);

   return 0;
}
