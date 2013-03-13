#include "vfml.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/times.h>
#include <time.h>
#include <math.h>

char  *gFileStem = "DF";
char  *gSourceDirectory = ".";
char  *gCommand = "";
int   gFoldCount = 10;
int   gSeed = -1;
char  *gTmpFileName = "xvalidate-tmp.out";
int   gMessageLevel = 0;
int   gGotCommand = 0;
int   gTableOutput = 0;

/* To add later */
int   gShuffleData = 0;
int   gc45Format = 0;
int   gvfdtFormat = 1;


static void _printUsage(char  *name) {
   printf("%s: performs cross validation of a learner on a dataset.\n",
                                                                        name);

   printf("-f <filestem>\tSet the name of the dataset (default DF)\n");
   printf("-source <dir>\tSet the source data directory (default '.')\n");
   printf("-c <command>\tSet the learner command.  The name of the dataset\n");
   printf("\t\t   will be appended to the end of this string and used to\n");
   printf("\t\t   invoke the learner (REQUIRED ARGUMENT)\n");

   printf("-folds <n>\tSets the number of train/test sets to create (default 10)\n");
   printf("-seed <n>\tSets the random seed, multiple runs with the same seed will\n");
   printf("\t\t   produce the same datasets (defaults to a random seed)\n");
   printf("-table \t output results suitable for creating an accuracy table (this is mainly used by 'batchtest', but you may find it useful).\n");
   printf("-v\t\tCan be used multiple times to increase the debugging output\n");

   printf("\n*** Things that are near being supported ***\n");
   printf("-shuffle\tRandomize the dataset before folding it\n");
   printf("output formats\tsupport c4.5 and other learner formats\n");
}

static void _processArgs(int argc, char *argv[]) {
   int i;

   /* the last argument is the command to run */

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
      } else if(!strcmp(argv[i], "-c")) {
         gCommand = argv[i+1];
         gGotCommand = 1;
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-shuffle")) {
         printf("-shuffle is not supported yet\n");
         exit(0);
         gShuffleData = 1;
      } else if(!strcmp(argv[i], "-fc45")) {
         printf("learner output formats are not supported yet\n");
         exit(0);
         gc45Format = 1;
         gvfdtFormat = 0;
      } else if(!strcmp(argv[i], "-fvfdt")) {
         printf("learner output formats are not supported yet\n");
         exit(0);
         gc45Format = 0;
         gvfdtFormat = 1;
      } else if(!strcmp(argv[i], "-table")) {
         gTableOutput = 1;
      } else if(!strcmp(argv[i], "-v")) {
         gMessageLevel++;
      } else if(!strcmp(argv[i], "-folds")) {
         sscanf(argv[i+1], "%d", &gFoldCount);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-seed")) {
         sscanf(argv[i+1], "%d", &gSeed);
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

   if(!gGotCommand) {
      printf("*** Didn't get the required -c option\n");
      _printUsage(argv[0]);
      exit(0);
   }

   if(gMessageLevel >= 1) {
      printf("Stem: %s\n", gFileStem);
      printf("Source: %s\n", gSourceDirectory);
      printf("Command %s\n", gCommand);
      printf("folds %d\n", gFoldCount);
      printf("seed %d\n", gSeed);
   }
}

int main(int argc, char *argv[]) {
   char command[500];
   int i;

   FILE *input;

   float error;
   float size;

   double errorSum;
   double squaredErrorSum;
   double meanError;

   double sizeSum;
   double squaredSizeSum;
   double meanSize;

   double utimeSum;
   double squaredUtimeSum;
   double meanUtime;

   double stimeSum;
   double squaredStimeSum;
   double meanStime;


   //   float prePruneError, postPruneError;
   //   float prePruneAccuracy, postPruneAccuracy;
   //   int prePruneSize, postPruneSize;

   //   float prePruneAccuracySum, postPruneAccuracySum;
   //   int prePruneSizeSum, postPruneSizeSum;

   struct tms startTime;
   struct tms finishTime;

   _processArgs(argc, argv);

   /* set up the random seed */
   RandomInit();
   if(gSeed != -1) {
      RandomSeed(gSeed);
   } else {
      gSeed = RandomRange(1, 10000);
      printf("Running with seed: %d\n", gSeed);
      RandomSeed(gSeed);
   }


   //   if(gShuffleData) {
   //      sprintf(command, "shuffledata -f %s", gFileStem, i);
   //      gFileStem = "shuffled";
   //      if(gMessageLevel >= 1) {
   //         printf("%s\n", command);
   //      }
   //      system(command);
   //   }


   /* do the folding */
   sprintf(command, "folddata -source %s -f %s -folds %d -seed %d", 
                    gSourceDirectory, gFileStem, gFoldCount, gSeed);
   if(gMessageLevel >= 1) {
      printf("%s\n", command);
   }
   system(command);

   utimeSum = 0;
   squaredUtimeSum = 0;
   stimeSum = 0;
   squaredStimeSum = 0;

   for(i = 0 ; i < gFoldCount ; i++) {
     //      if(gc45Format) {
     //         sprintf(command, "%s -u -f %s%d | grep \"<<\" >> %s", gCommand,
     //                    gFileStem, i, gTmpFileName);
     //      } else {

      sprintf(command, "%s %s%d >> %s", gCommand,
                       gFileStem, i, gTmpFileName);
      if(gMessageLevel >= 1) {
         printf("%s\n", command);
      }

      times(&startTime);
      system(command);
      times(&finishTime);

     #if defined(CYGNUS)
      utimeSum += ((double)(finishTime.tms_utime) - 
                   (double)(startTime.tms_utime)) / 100;
      stimeSum += ((double)(finishTime.tms_stime) - 
                   (double)(startTime.tms_stime)) / 100;

      squaredUtimeSum += (((double)(finishTime.tms_utime) - 
                   (double)(startTime.tms_utime)) / 100) *
                   (((double)(finishTime.tms_utime) - 
                   (double)(startTime.tms_utime)) / 100);
      squaredStimeSum += (((double)(finishTime.tms_stime) - 
                   (double)(startTime.tms_stime)) / 100) *
                   (((double)(finishTime.tms_stime) - 
                   (double)(startTime.tms_stime)) / 100);
     #else
      utimeSum += ((double)(finishTime.tms_cutime) - 
                   (double)(startTime.tms_cutime)) / 100;
      stimeSum += ((double)(finishTime.tms_cstime) - 
                   (double)(startTime.tms_cstime)) / 100;

      squaredUtimeSum += (((double)(finishTime.tms_cutime) - 
                   (double)(startTime.tms_cutime)) / 100) *
                   (((double)(finishTime.tms_cutime) - 
                   (double)(startTime.tms_cutime)) / 100);
      squaredStimeSum += (((double)(finishTime.tms_cstime) - 
                   (double)(startTime.tms_cstime)) / 100) *
                   (((double)(finishTime.tms_cstime) - 
                   (double)(startTime.tms_cstime)) / 100);
     #endif

      if(gMessageLevel >= 1) {
         printf("removing tmp files...\n");
      }

      sprintf(command, "%s%d.names", gFileStem, i);
      remove(command);
      sprintf(command, "%s%d.data", gFileStem, i);
      remove(command);
      sprintf(command, "%s%d.test", gFileStem, i);
      remove(command);


      //      if(gc45Format) {
      //         sprintf(command, "rm %s%d.unpruned %s%d.tree", gFileStem, i,
      //                   gFileStem, i);
      //
      //         if(gMessageLevel >= 1) {
      //            printf("%s\n", command);
      //         }
      //         system(command);
      //      }
   }

   errorSum = 0;
   squaredErrorSum = 0;
   sizeSum = 0;
   squaredSizeSum = 0;

   //   prePruneAccuracySum = 0;
   //   prePruneSizeSum = 0;
   //   postPruneAccuracySum = 0;
   //   postPruneSizeSum  = 0;

   input = fopen(gTmpFileName, "r");
   for(i = 0 ; i < gFoldCount ; i++) {
        //if(gc45Format) {
	// HERE something odd, I need to skip some lines of c45 I think
	//         fgets(buffer, 500, input);
        //   fscanf(input, "%d %*d ( %f %% ) %d %*d( %f %% ) ( %*f %% ) <<",
        //             &prePruneSize, &prePruneError,
        //           &postPruneSize, &postPruneError);
        //  prePruneAccuracy = 100.0 - prePruneError;
        //  postPruneAccuracy = 100.0 - postPruneError;
        //} else {
      fscanf(input, "%f %f", &error, &size);
      if(gMessageLevel >=1 ) {
         printf("Errors %f Size %f\n", error, size);
      }
        //}

      errorSum += error;
      squaredErrorSum += error * error;

      sizeSum += size;
      squaredSizeSum += size * size;
	 //      prePruneAccuracySum += prePruneAccuracy;
	 //      postPruneAccuracySum += postPruneAccuracy;
	 //      prePruneSizeSum += prePruneSize;
	 //      postPruneSizeSum += postPruneSize;
   }

   //   if(gc45Format) {
   // printf("%.3f\t%d\t%.3f\t%d\t%.3f\t%.3f\n",
   //                       prePruneAccuracySum / gFoldCount,
   //                       prePruneSizeSum / gFoldCount,
   //                      postPruneAccuracySum / gFoldCount,
   //                       postPruneSizeSum / gFoldCount,
   //                  (((double)(finishTime.tms_cutime) - 
   //                    (double)(startTime.tms_cutime)) / 100) / gFoldCount,
   //                  (((double)(finishTime.tms_cstime) - 
   //                    (double)(startTime.tms_cstime)) / 100) / gFoldCount);
   //} else {

   meanError = errorSum / gFoldCount;
   meanSize = sizeSum / gFoldCount;
   meanUtime = utimeSum / gFoldCount;
   meanStime = stimeSum / gFoldCount;

   if(!gTableOutput) {
      printf("%.3lf (%.3lf) \t%.3lf (%.3lf) \t%.3f (%.3f) \t%.3f (%.3f)\n",
                      meanError,
                      sqrt((squaredErrorSum / gFoldCount) -
                              meanError * meanError) / sqrt(gFoldCount), 
                      meanSize,
                      sqrt((squaredSizeSum / gFoldCount) -
                              meanSize * meanSize) / sqrt(gFoldCount),
                      meanUtime,
                      sqrt((squaredUtimeSum / gFoldCount) -
                              meanUtime * meanUtime) / sqrt(gFoldCount),
                      meanStime,
                      sqrt((squaredStimeSum / gFoldCount) -
                              meanStime * meanStime) / sqrt(gFoldCount));
   } else {
      printf("%.3lf %.3lf\t", meanError, sqrt((squaredErrorSum / gFoldCount) -
                              meanError * meanError) / sqrt(gFoldCount));

   }
   //}

   sprintf(command, "%s", gTmpFileName);
   remove(command);


   //   if(gShuffleData) {
   //      /* clean up if we shuffled */
   //      sprintf(command, "rm shuffled.names shuffled.data");
   //      if(gMessageLevel >= 1) {
   //         printf("%s\n", command);
   //      }
   //
   //      system(command);
   //   }
   return 0;
}
