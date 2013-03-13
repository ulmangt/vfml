#include "vfml.h"

#include <stdio.h>
#include <string.h>

char  *gDataFile = "datasets";
char  *gLearnerFile = "learners";
int   gFoldCount = 10;
long  gSeed = -1;
int   gMessageLevel = 0;
int   gTableOutput = 0;

static void _printUsage(char  *name) {
   printf("%s: Compares a collection of learners on a collection of datasets.\n",
                                                                        name);
   printf("     with cross-validation.\n");

   printf("-data <datafile>\tSets the file that contains pointers to the datasets\n");
   printf("\t\t   The format has one dataset description per line, each is:\n");
   printf("\t\t      [path to directory] :: [filestem]\n");
   printf("\t\t   (defaults to 'datasets')\n");

   printf("-learn <learnerfile>\tSets the file that contains descriptions of \n");
   printf("\t\t   the learners.  The format has one learner per line, each is:\n");
   printf("\t\t      [path to learner followed by arguments]\n");
   printf("\t\t   the filestems will be appended after the contents of each line.\n");
   printf("\t\t   (defaults to 'learners')\n");

   printf("-folds <n>\tSets the number of train/test sets to create (default 10)\n");
   printf("-seed <n>\tSets the random seed, multiple runs with the same seed will\n");
   printf("\t\t  produce the same cross-validation sets (defaults to random seed)\n");
   printf("-table \t output a table with 1 row per data set, and one column for learner with each column being <error-rate> <std-dev>\n");
   printf("-v\t\tCan be used multiple times to increase the debugging output\n");
}

static void _processArgs(int argc, char *argv[]) {
   int i;

   /* the last argument is the command to run */

   /* HERE on the ones that use the next arg make sure it is there */
   for(i = 1 ; i < argc ; i++) {
      if(!strcmp(argv[i], "-data")) {
         gDataFile = argv[i+1];
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-learn")) {
         gLearnerFile = argv[i+1];
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-v")) {
         gMessageLevel++;
      } else if(!strcmp(argv[i], "-table")) {
         gTableOutput = 1;;
      } else if(!strcmp(argv[i], "-folds")) {
         sscanf(argv[i+1], "%d", &gFoldCount);
         /* ignore the next argument */
         i++;
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
      printf("Dataset File: %s\n", gDataFile);
      printf("Learner File: %s\n", gLearnerFile);
      printf("folds %d\n", gFoldCount);
      printf("seed %ld\n", gSeed);
   }
}

int main(int argc, char *argv[]) {
   VoidAListPtr datasets = VALNew();
   VoidAListPtr stems = VALNew();
   VoidAListPtr learners = VALNew();


   char command[500];
   char buffer[500];
   char *tmp, *stem;
   int len;
   int i, j;

   FILE *input;

   _processArgs(argc, argv);

   /* set up the random seed */
   RandomInit();
   if(gSeed != -1) {
      RandomSeed(gSeed);
   } else {
      gSeed = RandomRange(1, 10000);
      if(!gTableOutput) {
         printf("Running with seed: %ld\n", gSeed);
      }

      RandomSeed(gSeed);
   }


   input = fopen(gDataFile, "r");
   DebugError(input == 0, "Unable to open the datasets file");
   while(fgets(buffer, 500, input)) {
      if(buffer[0] == '#') {
         continue;
      } else if(buffer[0] == '\n') {
         continue;
      }

      stem = strstr(buffer, "::");
      DebugError(!stem, buffer);
      stem[-1] = '\0';
      stem += 3;
      
      tmp = MNewPtr(sizeof(char) * (strlen(buffer) + 1));
      strcpy(tmp, buffer);
      VALAppend(datasets, tmp);

      /* now strip off the newline */
      len = strlen(stem);
      if(stem[len - 1] == '\n') {
         stem[len - 1] = '\0';
      }

      tmp = MNewPtr(sizeof(char) * (len + 1));
      strcpy(tmp, stem);
      VALAppend(stems, tmp);
   }


   input = fopen(gLearnerFile, "r");
   DebugError(input == 0, "Unable to open the learner file");
   while(fgets(buffer, 500, input)) {
      if(buffer[0] == '#') {
         continue;
      } else if(buffer[0] == '\n') {
         continue;
      }

      /* now strip off the newline */
      len = strlen(buffer);
      if(buffer[len - 1] == '\n') {
         buffer[len - 1] = '\0';
      }
      
      tmp = MNewPtr(sizeof(char) * (len + 1));
      strcpy(tmp, buffer);
      VALAppend(learners, tmp);
   }


   for(i = 0 ; i < VALLength(datasets) ; i++) {
      if(!gTableOutput) {
         printf("------- %s -------\n", (char *)VALIndex(stems, i));
         fflush(stdout);
      } else {
         printf("%s: ", (char *)VALIndex(stems, i));
         fflush(stdout);
      }
      for(j = 0 ; j < VALLength(learners) ; j++) {

         if(!gTableOutput) {
            printf("'%s' :\n", (char *)VALIndex(learners, j));
            fflush(stdout);

            sprintf(command, "xvalidate -f %s -source %s -seed %ld -folds %d -c \"%s\"",
              (char *)VALIndex(stems, i), (char *)VALIndex(datasets, i), gSeed, gFoldCount,
              (char *)VALIndex(learners, j));
         } else {
            sprintf(command, "xvalidate -table -f %s -source %s -seed %ld -folds %d -c \"%s\"",
              (char *)VALIndex(stems, i), (char *)VALIndex(datasets, i), gSeed, gFoldCount,
              (char *)VALIndex(learners, j));
         }

         if(gMessageLevel >= 1) {
            printf("%s\n", command);
            fflush(stdout);
         }
         system(command);
      }
      if(gTableOutput) {
         printf("\n");
         fflush(stdout);
      }
   }

   return 0;
}
