#include "vfml.h"
#include <stdio.h>
#include <string.h>

#include <sys/times.h>
#include <time.h>

char *gFileStem = "DF";
char *gSourceDirectory = ".";
int   gDoTests = 0;
int   gMessageLevel = 0;
int   gNumClusters = -1;
float gConvergeThreshold = 0.001;


int   gIterativeResults = 0;

static void _printUsage(char  *name) {
   printf("%s : Simple k-means clustering\n", name);

   printf("-f <filestem>\t Set the name of the dataset (default DF)\n");
   printf("-source <dir>\t Set the source data directory (default '.')\n");
   printf("-clusters <numClusters>\t Set the num clusters to find (REQUIRED)\n");
   printf("-threshold <convergeThreshold>\t Iterate until all clusters move less\n\t\t than the specified threshold (default 0.001)\n");
   printf("-u\t\t Test comparing to centroids in <stem>.test\n");

   printf("-v\t\t Can be used multiple times to increase the debugging output\n");
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
      } else if(!strcmp(argv[i], "-clusters")) {
        sscanf(argv[i+1], "%d", &gNumClusters);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-threshold")) {
        sscanf(argv[i+1], "%f", &gConvergeThreshold);
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

   if(gNumClusters < 0) {
      printf("Didn't supply the required '-clusters' argument\n");
      exit(0);
   }

   if(gMessageLevel >= 1) {
      printf("Stem: %s\n", gFileStem);
      printf("Source: %s\n", gSourceDirectory);

      if(gDoTests) {
         printf("Running tests\n");
      }
   }
}

static float _MatchCentersGetDistance(VoidAListPtr learnedCenters, VoidAListPtr testCenters) {
   /* perfom a 1 to 1 matching between the learned and test centers.
        change the class lables of the learnedCenters to correspond to
        the closest testCenters.  Do a greedy assignment that tries to
        minimize the total distance between assigned pairs */
   int i, j, init;
   double bestDistance, thisDistance;
   int lBestIndex, tBestIndex;
   VoidAListPtr unLearned = VALNew();
   VoidAListPtr unTest = VALNew();
   float totalDistance = 0.0;

   for(i = 0 ; i < VALLength(learnedCenters) ; i++) {
      VALAppend(unLearned, VALIndex(learnedCenters, i));
   }

   for(i = 0 ; i < VALLength(testCenters) ; i++) {
      VALAppend(unTest, VALIndex(testCenters, i));
   }

   while(VALLength(unLearned) > 0) {
      init = 0; bestDistance = 100.0;
      lBestIndex = tBestIndex = -1;
      for(i = 0 ; i < VALLength(unLearned) ; i++) {
         for(j = 0 ; j < VALLength(unTest) ; j++) {
            thisDistance = ExampleDistance(VALIndex(unLearned, i),
                                           VALIndex(unTest, j));
            if(thisDistance < bestDistance || !init) {
               init = 1;
               bestDistance = thisDistance;
               lBestIndex = i;
               tBestIndex = j;
            }
         }
      }
      if(gMessageLevel > 2) {
         printf("\tMatched cluster %d distance %lf\n", ExampleGetClass(VALIndex(unTest, tBestIndex)), ExampleDistance(VALIndex(unLearned, lBestIndex), VALIndex(unTest, tBestIndex)));
      }

      totalDistance += bestDistance * bestDistance;
      ExampleSetClass(VALRemove(unLearned, lBestIndex),
               ExampleGetClass(VALRemove(unTest, tBestIndex)));
   }

   VALFree(unLearned);
   VALFree(unTest);

   return totalDistance;
}

static int _FindClosestCenterIndex(ExamplePtr e, VoidAListPtr centers) {
   int i;
   double bestDistance, thisDistance;
   int bestIndex;

   bestIndex = 0;
   bestDistance = ExampleDistance(e, VLIndex(centers, 0));

   for(i = 1 ; i < VLLength(centers) ; i++) {
      thisDistance = ExampleDistance(e, VLIndex(centers, i));
      if(thisDistance < bestDistance) {
         bestDistance = thisDistance;
         bestIndex = i;
      }
   }

   return bestIndex;
}

//static ExamplePtr _FindClosestCenter(ExamplePtr e, VoidAListPtr centers) {
//   return VALIndex(centers, _FindClosestCenterIndex(e, centers));
//}

static void _doTests(ExampleSpecPtr es, VoidAListPtr learnedCenters, long learnCount, long learnTime, int finalOutput) {
   char fileNames[255];
   FILE *testCentersIn, *centersOut;
   ExamplePtr tc;
   VoidAListPtr testCenters;
   long i;

   float distance;


   /* load the test centers */
   testCenters = VALNew();

   sprintf(fileNames, "%s/%s.test", gSourceDirectory, gFileStem);
   testCentersIn = fopen(fileNames, "r");
   DebugError(testCentersIn == 0, "Unable to open the .test file");
      
   if(gMessageLevel >= 1) {
      printf("reading the test centers file...\n");
   }

   tc = ExampleRead(testCentersIn, es);
   while(tc != 0) {
      VALAppend(testCenters, tc);
      tc = ExampleRead(testCentersIn, es);
   }
   fclose(testCentersIn);
   
   /* Match learned centers with the test centers */
   distance = _MatchCentersGetDistance(learnedCenters, testCenters);


   /* just output the distance between matched centers */
   printf("%ld\t%.4f\t%.2lf\n",
             learnCount,
             distance,
             ((double)learnTime) / 100);
   fflush(stdout); 

   if(finalOutput) {//gOutputCenters) {
      sprintf(fileNames, "%s.centers", gFileStem);
      centersOut = fopen(fileNames, "w");

      for(i = 0 ; i < VALLength(learnedCenters) ; i++) {
         ExampleWrite(VALIndex(learnedCenters, i), centersOut);
//         ExampleWrite(VALIndex(learnedCenters, i), stdout);
      }
//      printf("------------------\n");
      fclose(centersOut);
   }

   /* free the test centers */
   for(i = 0 ; i < VALLength(testCenters) ; i++) {
      ExampleFree(VALIndex(testCenters, i));
   }
   VALFree(testCenters);
}

static int _DoClusterIterationDidConverge(VoidAListPtr centers, FILE *data, ExampleSpecPtr es) {
   ExamplePtr e, newCenter;
   VoidAListPtr sumsList, newCenters;
   int converged;
   int centerIndex;
   double *sums;
   long *num;
   int i,j;
   float thisDistance;

   /* make the cluster sums holders */
   num = MNewPtr(sizeof(long) * VALLength(centers));
   sumsList = VALNew();
   for(i = 0 ; i < VALLength(centers) ; i++) {
      num[i] = 0;
      sums = MNewPtr(sizeof(double) * ExampleSpecGetNumAttributes(es));
      for(j = 0 ; j < ExampleSpecGetNumAttributes(es) ; j++) {
         sums[j] = 0;
      }
      VALAppend(sumsList, sums);
   }


   e = ExampleRead(data, es);
   while(e != 0) {
      /* find the nearest cluster center */
      centerIndex = _FindClosestCenterIndex(e, centers);

      num[centerIndex]++;

      sums = VALIndex(sumsList, centerIndex);
      for(i = 0 ; i < ExampleSpecGetNumAttributes(es) ; i++) {
         if(!ExampleIsAttributeUnknown(e, i)) {
            if(ExampleIsAttributeContinuous(e, i)) {
               sums[i] += ExampleGetContinuousAttributeValue(e, i);
            } else {
               /* HERE what to do about discrete attributes */
            }
         }
      }

      ExampleFree(e);
      e = ExampleRead(data, es);
   }
   
   /* make the list of new centers */
   newCenters = VALNew();
   for(i = 0 ; i < VALLength(sumsList) ; i++) {
      newCenter = ExampleNew(es);
      sums = VALIndex(sumsList, i);

      if(gMessageLevel > 1 && num[i] == 0) {
         printf("No examples assigned to: ");
         ExampleWrite(VALIndex(centers, i), stdout);
      }

      for(j = 0 ; j < ExampleSpecGetNumAttributes(es) ; j++) {
         if(ExampleSpecIsAttributeContinuous(es, j)) {
            if(num[i] > 0) {
               ExampleSetContinuousAttributeValue(newCenter, j,
                               sums[j] / (double)num[i]);
            } else {
               /* leave the attribute the same */
               ExampleSetContinuousAttributeValue(newCenter, j, 
                 ExampleGetContinuousAttributeValue(VALIndex(centers, i),j));
            }
         } else {
            /* HERE what to do about discrete attributes */
         }
      }
      VALAppend(newCenters, newCenter);
   }

   /* check to see if things have converged */
   converged = 1;
   for(i = 0 ; i < VALLength(centers) && converged ; i++) {
      /* HERE put in a configurable threshold */
      thisDistance = ExampleDistance(VALIndex(centers, i),
                                      VALIndex(newCenters, i));
      if(thisDistance > gConvergeThreshold) {
         converged = 0;
      }
      if(gMessageLevel > 2) {
         printf("   cluster %d moved by %lf\n", i, thisDistance);
      }
   }

   /* update the list of centers & free the tmpCenters */
   for(i = 0 ; i < VALLength(centers) ; i++) {
      ExampleFree(VALIndex(centers, i));
      VALSet(centers, i, VALIndex(newCenters, i));
   }
   VALFree(newCenters);

   /* free the memory used for the counters */
   for(i = 0 ; i < VALLength(sumsList) ; i++) {
      MFreePtr(VALIndex(sumsList, i));
   }
   VALFree(sumsList);
   MFreePtr(num);

   return converged;
}

static void _PickInitialCentroids(ExampleSpecPtr es, VoidAListPtr centroids,
                                       FILE *data) {
   /* pick the first unique 'gNumClusters' points from the dataset to
            be centroids  HERE should I add some randomness to this? */
   int i;
   int used;
   ExamplePtr e;

   while(VALLength(centroids) < gNumClusters) {
      e = ExampleRead(data, es);
      DebugError(e == 0, "Unable to get enough unique initial centroids");

      /* make sure this isn't already in the list */
      used = 0;
      for(i = 0 ; i < VALLength(centroids) && !used ; i++) {
         if(ExampleDistance(e, VALIndex(centroids, i)) == 0) {
           used = 1;
         }
      }

      /* if it's ok then use it */
      if(!used) {
         VALAppend(centroids, e);
      } else {
         ExampleFree(e);
      }
   }
}

int main(int argc, char *argv[]) {
   char fileNames[255];

   FILE *exampleIn;
   ExampleSpecPtr es;
   VoidListPtr centers;   

   long learnTime;
   int iteration = 0;


   struct tms starttime;
   struct tms endtime;

   _processArgs(argc, argv);

   sprintf(fileNames, "%s/%s.names", gSourceDirectory, gFileStem);
   es = ExampleSpecRead(fileNames);
   DebugError(es == 0, "Unable to open the .names file");

   RandomInit();

   /* Pick the initial centroids from the dataset */
   centers = VALNew();
   sprintf(fileNames, "%s/%s.data", gSourceDirectory, gFileStem);
   exampleIn = fopen(fileNames, "r");
   DebugError(exampleIn == 0, "Unable to open the .data file");
   _PickInitialCentroids(es, centers, exampleIn);
   fclose(exampleIn);

   if(gMessageLevel >= 1) {
      printf("allocation %ld\n", MGetTotalAllocation());
   }
   learnTime = 0;

   sprintf(fileNames, "%s/%s.data", gSourceDirectory, gFileStem);
   exampleIn = fopen(fileNames, "r");
   DebugError(exampleIn == 0, "Unable to open the .data file");

   times(&starttime);

   while(!_DoClusterIterationDidConverge(centers, exampleIn, es)) {
      iteration++;
      times(&endtime);
      learnTime += endtime.tms_utime - starttime.tms_utime;


      /* reset the file for the next iteration */
      fclose(exampleIn);
      sprintf(fileNames, "%s/%s.data", gSourceDirectory, gFileStem);
      exampleIn = fopen(fileNames, "r");
      DebugError(exampleIn == 0, "Unable to open the .data file");

      if(gDoTests && gMessageLevel >= 1) {
         _doTests(es, centers, iteration, learnTime, 0);
      }

      /* reset the timer for the next iteration */
      times(&starttime);
   }
   iteration++;
   times(&endtime);
   learnTime += endtime.tms_utime - starttime.tms_utime;

   if(gDoTests) {
      _doTests(es, centers, iteration, learnTime, 1);
   }

   return 0;
}


