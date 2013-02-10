#include "vfml.h"

#include "vfem-engine.h"

#include <stdio.h>
#include <string.h>

#include <sys/times.h>
#include <time.h>

#include <math.h>

char *gFileStem        = "DF";
char *gSourceDirectory = ".";
int   gDoTests         = 0;
int   gMessageLevel    = 0;
int   gNumClusters     = -1;
float gDelta           = 0.05;
float gErrorTarget     = 0.01;
float gThisErrorTarget = 0.01;
int   gEstimatedNumIterations = 5;
int   gBatch                  = 0;
long  gMaxExamplesPerIteration = 1000000000;
int   gInitNumber      = 1;
int   gFancyStop              = 1;
float gConvergeDelta          = 0.001;
int   gSeed             = -1;
float gR = 0;
float gAssignErrorScale = 1.0;
int   gAllowBadConverge = 0;
float gSigmaSquare      = .05;
int   gStdin            = 0;
int   gIterativeResults = 0;
int   gTestOnTrain      = 0; /* default test total center-matching distance */
int   gNormalApprox     = 0;
long         gDBSize                 = 0;

/* HERE add parameters for these */
int          gDoEarlyStop            = 0;
int          gLoadCenters            = 0;
int          gReassignCentersEachRound = 0;
int          gUseGeoffBound          = 1;

/* HERE some hackish globals & stuff*/
VoidAListPtr gStatsList;
int          gD;
int          gIteration = 0;
int          gRound     = 0;
int          gTotalExamplesSeen = 0;
float        gNeededDelta;
float        gTargetEkd;
long         gN;
float        *gIterationNs = 0;
int          gNumIterationNs;

static void _printUsage(char  *name) {
   printf("%s : Very Fast Expectation Maximization\n", name);

   printf("-f <filestem>\t Set the name of the dataset (default DF)\n");
   printf("-source <dir>\t Set the source data directory (default '.')\n");
   printf("-clusters <numClusters>\t Set the num clusters to find (REQUIRED)\n");
   printf("-variance <value>\t The variance of the Gaussians, the current\n\t\t version of EM needs this as a parameter (default 0.05)\n");
   printf("-assignErrorScale <scale>\t Loosen bound by scaling the assignment\n\t\t part of it by the scale factor (default 1.0)\n");
   printf("-delta <delta>\t Find final solution with confidence (1 - <delta>)\n\t\t (default 5%%)\n");
   printf("-epsilon <epsilon>\t The maximum distance between a learned\n\t\t centroid and the coresponding infinite\n\t\t data centroid (default .01)\n");
   printf("-converge <distance>\t Stop when distance centroids move between\n\t\t iterations squared is less than <distance>  (default .001)\n");
   printf("-batch\t Do a traditional kmeans run on the whole input file.  Doesn't\n\t\t make sense to combine with -stdin (default off)\n");
   printf("-init <num>\t use the <num>th valid assignment of initial centroids\n\t\t (default 1)\n");
   printf("-maxPerIteration <num> \t use no more than num examples per\n\t\t iteration (default 1,000,000,000)\n");
   printf("-l <number>\t The estimated # of iterations to converge if you\n\t\t guess wrong and aren't using batch VFEM will fix it for you\n\t\t and do an extra round (default 10)\n");
   printf("-seed <s>\t Seed to use picking initial centers (default random)\n");
   printf("-progressive\t Don't use the Lagrange based optimization\n\t\t to pick the # of samples at each iteration of the\n\t\t next round (default use the optimization)\n");
   printf("-tightBound\t Use a tighter assignment error bound (requires\n\t\t a second pass over the dataset) (default use a\n\t\t looser one-pass bound)\n");
   printf("-allowBadConverge\t Allows VFEM to converge at a time when\n\t\t em wouldn't (default off).\n");
   printf("-r <range>\t The maximum range between pairs of examples\n\t\t (default assume the range of each dimension is 0 - 1.0\n\t\t and calculate it from that)\n");
   printf("-stdin \t\t Reads training examples as a stream from stdin\n\t\t instead of from <stem>.data (default off)\n");
   printf("-testOnTrain \t loss is the sum of the square of the distances\n\t\t from allu training examples to their assigned\n\t\t centroid (default compare learned centroids to\n\t\t centroids in <stem>.test)\n");
   printf("-normalApprox \t use a normal approximation of the Hoeffding\n\t\t bound (default use hoeffding bound)\n");
   printf("-loadCenters \t load initial centroids from <stem>.centers\n\t\t (default use random based on -init and -seed)\n");
   printf("-u\t\t Output results after each iteration\n");
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
      } else if(!strcmp(argv[i], "-testOnTrain")) {
         gTestOnTrain = 1;
      } else if(!strcmp(argv[i], "-clusters")) {
         sscanf(argv[i+1], "%d", &gNumClusters);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-dbSize")) {
         sscanf(argv[i+1], "%ld", &gDBSize);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-delta")) {
         sscanf(argv[i+1], "%f", &gDelta);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-converge")) {
         sscanf(argv[i+1], "%f", &gConvergeDelta);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-r")) {
         sscanf(argv[i+1], "%f", &gR);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-assignErrorScale")) {
         sscanf(argv[i+1], "%f", &gAssignErrorScale);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-variance")) {
         sscanf(argv[i+1], "%f", &gSigmaSquare);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-epsilon")) {
         sscanf(argv[i+1], "%f", &gErrorTarget);
         gThisErrorTarget = gErrorTarget;
         /* ignore the next argument */
         i++;
//      } else if(!strcmp(argv[i], "-n")) {
//         sscanf(argv[i+1], "%ld", &gN);
//         /* ignore the next argument */
//         i++;
      } else if(!strcmp(argv[i], "-maxPerIteration")) {
         sscanf(argv[i+1], "%ld", &gMaxExamplesPerIteration);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-batch")) {
         gBatch = 1;
      } else if(!strcmp(argv[i], "-allowBadConverge")) {
         gAllowBadConverge = 1;
      } else if(!strcmp(argv[i], "-progressive")) {
         gFancyStop = 0;
      } else if(!strcmp(argv[i], "-tightBound")) {
         gUseGeoffBound = 0;
      } else if(!strcmp(argv[i], "-init")) {
         sscanf(argv[i+1], "%d", &gInitNumber);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-l")) {
         sscanf(argv[i+1], "%d", &gEstimatedNumIterations);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-seed")) {
         sscanf(argv[i+1], "%d", &gSeed);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-normalApprox")) {
         gNormalApprox = 1;
      } else if(!strcmp(argv[i], "-loadCenters")) {
         gLoadCenters = 1;
      } else if(!strcmp(argv[i], "-v")) {
         gMessageLevel++;
      } else if(!strcmp(argv[i], "-h")) {
         _printUsage(argv[0]);
         exit(0);
      } else if(!strcmp(argv[i], "-stdin")) {
         gStdin = 1;
      } else {
         printf("Unknown argument: %s.  use -h for help\n", argv[i]);
         exit(0);
      }
   }

   if(gNumClusters < 0) {
      printf("Didn't supply the required '-clusters' argument\n");
      exit(0);
   }

   if(gDBSize <= 0) {
      printf("Didn't supply the required '-dbSize' argument\n");
      exit(0);
   }

   if(gMessageLevel >= 1) {
      printf("Stem: %s\n", gFileStem);
      printf("Source: %s\n", gSourceDirectory);

      if(gStdin) {
         printf("Reading examples from standard in.\n");
      }
      if(gDoTests) {
         printf("Running tests\n");
      }
      printf("DB Size %ld\n", gDBSize);
   }
}

static float _MatchCentersGetDistanceSquare(VoidAListPtr learnedCenters, VoidAListPtr testCenters) {
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

   bestDistance = lBestIndex = tBestIndex = 0;
   while(VALLength(unLearned) > 0) {
      init = 0;
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
      if(gMessageLevel > 0) {
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

static ExamplePtr _FindClosestCenter(ExamplePtr e, VoidAListPtr centers) {
   return VALIndex(centers, _FindClosestCenterIndex(e, centers));
}

static float _CalculateIDKMEarlyBound(void) {
   int i, j, k;
   float maxError, thisError, bound, cError;
   IterationStatsPtr isL, isI;
   ExamplePtr eL, eI;

   isL = VALIndex(gStatsList, VALLength(gStatsList) - 1);

   maxError = 0;
   for(i = 0 ; i < VALLength(gStatsList) ; i++) {
      isI = VALIndex(gStatsList, i);

      if(isI->possibleIDConverge) {
         thisError = 0;
         for(j = 0 ; j < VALLength(isI->centroids) ; j++) {
            eL = VALIndex(isL->centroids, j);
            eI = VALIndex(isI->centroids, j);
            cError = 0;
            for(k = 0 ; k < ExampleGetNumAttributes(eL) ; k++) {
               /* HERE fix for discrete */
               bound = ExampleGetContinuousAttributeValue(eL, k) -
                          ExampleGetContinuousAttributeValue(eI, k) +
                          IterationStatsErrorBoundDimension(isI, j, k);
               cError += pow(bound, 2);
            }
            thisError += cError;
            //printf("i%d c%d cError %f totalError %f\n",
            //         i, j, cError, thisError);
         }

         if(thisError > maxError) {
            maxError = thisError;
         }
      }
   }

   return maxError;
}

static float _CalculateErrorBound(void) {
   int i;
   float maxError = 0;
   float earlyError;
   IterationStatsPtr isL;

   isL = VALIndex(gStatsList, VALLength(gStatsList) - 1);

   /* find the error from the final iteration */
   for(i = 0 ; i < VALLength(isL->centroids) ; i++) {
      maxError += pow(isL->lastBound[i], 2);
      if(gMessageLevel > 2) {
         printf("max ekd %.4f sum bnd^2 %.4f bnd%d %.4f bnd^2%d %.4f\n",
            isL->maxEkd, maxError, i, isL->lastBound[i], i, 
              pow(isL->lastBound[i], 2));
      }
   }

   if(!gAllowBadConverge) {
      earlyError = _CalculateIDKMEarlyBound();

      if(earlyError > maxError) {
         if(gMessageLevel > 1) {
            printf("early stop error (%.5f) greater than El (%.5f)\n", 
                 earlyError, maxError);
         }

         maxError = earlyError;
      }
   }

   return maxError;
}

static int _WhenEMConverged(void) {
   int i;
   IterationStatsPtr is;

   for(i = 0 ; i < VLLength(gStatsList) ; i++) {
      is = VLIndex(gStatsList, i);
      if(is->wouldEMConverge) {
         return i;
      }
   }

   return VLLength(gStatsList) - 1;
}

static void _doTests(ExampleSpecPtr es, VoidAListPtr learnedCenters, long learnCount, long learnTime, int finalOutput) {
   char fileNames[255];
   FILE *exampleIn, *testCentersIn, *centersOut;
   ExamplePtr e, tc, lc;
   VoidAListPtr testCenters;
   long i;
   float loss;
   float bound;
   int foundBound, guarenteeIDConverge, lastFoundBound;
   long tested = 0;


   foundBound = 0;
   if(VALLength(gStatsList) > 0) {
      lastFoundBound = ((IterationStatsPtr)VALIndex(gStatsList,
                VALLength(gStatsList) - 1))->foundBound;
      guarenteeIDConverge = ((IterationStatsPtr)VALIndex(gStatsList,
                     VALLength(gStatsList) - 1))->guarenteeIDConverge;
      if(lastFoundBound) {
         if(guarenteeIDConverge) {
            foundBound = 1;
            if(gMessageLevel >= 1) {
               printf("Have a bound and ID would converge\n");
            }
         } else if(((IterationStatsPtr)VALIndex(gStatsList,
                        VALLength(gStatsList) - 1))->wouldEMConverge &&
                    gAllowBadConverge) {

            if(gMessageLevel >= 1) {
               printf("Have a bound and ID may or may not converge, we converge.\n");
            }
            foundBound = 1;
         } else {
            if(gMessageLevel >= 1) {
               printf("Have a bound and ID may or may not converge, we don't converge.\n");
            }

         }
      }
   }
   if(foundBound) {
      bound = _CalculateErrorBound();
   } else {
      bound = -1;
   }

   if(!gTestOnTrain) {
      /* just output the distance between matched centers */
      /* load the test centers */
      testCenters = VALNew();

      sprintf(fileNames, "%s/%s.test", gSourceDirectory, gFileStem);
      testCentersIn = fopen(fileNames, "r");
      DebugError(testCentersIn == 0, "Unable to open the .test file");
      
      if(gMessageLevel >= 2) {
         printf("reading the test centers file...\n");
      }

      tc = ExampleRead(testCentersIn, es);
      while(tc != 0) {
         VALAppend(testCenters, tc);
         tc = ExampleRead(testCentersIn, es);
      }
      fclose(testCentersIn);
   
      /* Match learned centers with the test centers */
      loss = _MatchCentersGetDistanceSquare(learnedCenters, testCenters);

      /* free the test centers */
      for(i = 0 ; i < VALLength(testCenters) ; i++) {
         ExampleFree(VALIndex(testCenters, i));
      }
      VALFree(testCenters);
   } else { /* Sum Square distance of example to assigned cluster */
      loss = 0;

      sprintf(fileNames, "%s/%s.data", gSourceDirectory, gFileStem);
      exampleIn = fopen(fileNames, "r");
      DebugError(exampleIn == 0, "Unable to open the .data file");
      
      if(gMessageLevel >= 1) {
         printf("opened test file, starting scan...\n");
      }

      e = ExampleRead(exampleIn, es);
      /* HERE only tests on the first 10k test examples? Parameter?? */
      while(e != 0 && tested < gMaxExamplesPerIteration) {
         tested++;
         lc = _FindClosestCenter(e, learnedCenters);
         loss += pow(ExampleDistance(e, lc), 2);
         ExampleFree(e);
         e = ExampleRead(exampleIn, es);
      }
      if(e != 0) {
         ExampleFree(e);
      }
      fclose(exampleIn);
   }


   if(finalOutput) {
      printf("%.4f\t0\n", loss);
   } else {
      if(foundBound) {
         if(bound < gThisErrorTarget) {
            printf("%d\t%ld\t%d\t%.6f\t%.6f\t%.2lf\n",
                gRound, learnCount, gTotalExamplesSeen,
                bound, loss, ((double)learnTime) / 100);
         } else {
            printf("%d\t%ld\t%d\t*%.6f\t%.6f\t%.2lf\n",
                gRound, learnCount, gTotalExamplesSeen,
                bound, loss, ((double)learnTime) / 100);
         }
      } else {
         if(gMessageLevel > 1) {
             printf("   No bound, Current bound estimate is %f guarenteed converge %d\n",
                       _CalculateErrorBound(),
                       ((IterationStatsPtr)VALIndex(gStatsList,
                            VALLength(gStatsList) - 1))->guarenteeIDConverge);
         }
         printf("%d\t%ld\t%d\t***\t%.6f\t%.2lf\n",
                gRound, learnCount, gTotalExamplesSeen,
                loss, ((double)learnTime) / 100);
      }
   }
   fflush(stdout); 


   if(0) {//gOutputCenters) {
      sprintf(fileNames, "%s-%lu.centers", gFileStem, learnCount);
      centersOut = fopen(fileNames, "w");

      for(i = 0 ; i < VALLength(learnedCenters) ; i++) {
         ExampleWrite(VALIndex(learnedCenters, i), centersOut);
//         ExampleWrite(VALIndex(learnedCenters, i), stdout);
      }
//      printf("------------------\n");
      fclose(centersOut);
   }

}

static int _CheckConverganceUpdateStats(IterationStatsPtr last, 
                                 IterationStatsPtr current) {
   float thisDistance;
   float bound, lowerBound, upperBound, clusterBound;
   float error;
   ExamplePtr eThis, eLast;
   int i, j;


   bound = 0;
   lowerBound = 0;
   upperBound = 0;

   for(i = 0 ; i < VALLength(last->centroids) ; i++) {
      eLast = VALIndex(last->centroids, i);
      eThis = VALIndex(current->centroids, i);
      clusterBound = 0;
      for(j = 0 ; j < ExampleGetNumAttributes(eThis) ; j++) {
         /* HERE fix for discrete ?? */
         thisDistance = ExampleGetContinuousAttributeValue(eLast, j) -
                    ExampleGetContinuousAttributeValue(eThis, j);
         if(thisDistance < 0) {
            thisDistance *= -1;
         }

         error = IterationStatsErrorBoundDimension(last, i, j) +
	   IterationStatsErrorBoundDimension(current, i, j);
         bound += pow(thisDistance, 2);
         clusterBound += pow(thisDistance, 2);
         lowerBound += pow(max(thisDistance - error, 0), 2);
         upperBound += pow(thisDistance + error, 2);
         if(gMessageLevel > 3) {
           printf("e: %.4f LossDeltas: dim %.4f sum %.4f min %.4f max %.4f\n", 
                error, bound, clusterBound, lowerBound, upperBound);
         }
      }
      if(gMessageLevel > 0) {
         printf("   cluster %d moved ^2 loss of %f\n", i, clusterBound);
      }
   }

   if(gMessageLevel > 1) {
      for(i = 0 ; i < VALLength(current->centroids) ; i++) {
         for(j = 0 ; j < VALLength(current->centroids) ; j++) {
            printf("%.3f  ", ExampleDistance(VALIndex(current->centroids, i),
                                        VALIndex(current->centroids, j)));
         }
         printf("\n");
      }
   }

   if(gMessageLevel > 0) {
      printf("   clusters moved [ %f - %f - %f ] tau %f\n",
                              lowerBound, bound, upperBound, gConvergeDelta);
   }

   if(bound <= gConvergeDelta / 3.0) {
      current->convergeVFEM = 1;
   }

   if(lowerBound <= gConvergeDelta) {
      current->possibleIDConverge = 1;

      if(bound <= gConvergeDelta) {
         current->wouldEMConverge = 1;
      }

      if(upperBound <= gConvergeDelta) { 
         current->guarenteeIDConverge = 1;
      } else if(gMessageLevel > 0) {
         printf("      IDEM may have or may not have converged.\n");
      }
   }

   if(gMessageLevel > 0) {
      printf("   converge info guarenteeID: %d possibleID: %d - EM / 3.0 this: %d last: %d\n", current->guarenteeIDConverge, current->possibleIDConverge, current->convergeVFEM, last->convergeVFEM);
   }

   if(gBatch || gAllowBadConverge) {
      if(gMessageLevel > 0 && gAllowBadConverge && current->wouldEMConverge) {
         printf("      found a potentially bad converge.\n");
      }
      return current->wouldEMConverge;
   } else {
      return current->guarenteeIDConverge || 
          (current->convergeVFEM && last->convergeVFEM);
   }
}

float AssignmentScaledDeltaMax(ExamplePtr e,
                          ExamplePtr centroid, ExamplePtr min, 
                           ExamplePtr max, float epsilon) {
   float observedDelta;

   observedDelta = ExampleDistance(e, centroid);

   // maximumDelta = max(ExampleDistance(e, min),
   //                     ExampleDistance(e, max));

   ///* deal with the assignErrorScale */
   //return observedDelta + (gAssignErrorScale * (maximumDelta - observedDelta));

   return observedDelta + epsilon;
}

static int _PointInBox(ExamplePtr e, ExamplePtr cMin, ExamplePtr cMax) {
   int i;

   for(i = 0 ; i < ExampleGetNumAttributes(e) ; i++) {
      if(ExampleGetContinuousAttributeValue(e, i) <
           ExampleGetContinuousAttributeValue(cMin, i) ||
         ExampleGetContinuousAttributeValue(e, i) >
           ExampleGetContinuousAttributeValue(cMax, i)) {
         return 0;
      }
   }

   return 1;
}

float AssignmentScaledDeltaMin(ExamplePtr e,
                          ExamplePtr centroid, ExamplePtr min, 
                           ExamplePtr max, float epsilon) {
   float observedDelta;

   observedDelta = ExampleDistance(e, centroid);

   //if(_PointInBox(e, min, max)) {
   //   minimumDelta = 0;
   //} else {
   //   minimumDelta = min(ExampleDistance(e, min),
   //           ExampleDistance(e, max));
   //}

   //return min(observedDelta - (gAssignErrorScale * 
   //                 (observedDelta - minimumDelta)), observedDelta);

   return max(observedDelta - epsilon, 0);
}

static void _RecordGeoffBoundInfo(ExamplePtr e, IterationStatsPtr is, ExampleSpecPtr es) {
   int i, j;
   ExamplePtr centroid, cMin, cMax;
   double denominator, numerator, weight;
   double *denomValues;

   /* HERE modify for negative Xs */
   /* do the W-Plusses */

   denomValues = MNewPtr(sizeof(double) * VALLength(is->centroids));

   denominator = 0;
   for(i = 0 ; i < VALLength(is->centroids) ; i++) {
      centroid = VLIndex(is->centroids, i);
      cMax = VLIndex(is->cMax, i);
      cMin = VLIndex(is->cMin, i);

      denomValues[i] = exp( (-1.0 / (2.0 * gSigmaSquare)) *
        pow(AssignmentScaledDeltaMax(e, centroid, cMin, cMax, 
                                             is->lastBound[i]), 2));
      denominator += denomValues[i];
   }

   for(i = 0 ; i < VALLength(is->centroids) ; i++) {
      centroid = VLIndex(is->centroids, i);
      cMax = VLIndex(is->cMax, i);
      cMin = VLIndex(is->cMin, i);

      numerator = exp( (-1.0 / (2.0 * gSigmaSquare)) *
         pow(AssignmentScaledDeltaMin(e, centroid, cMin, cMax,
                                             is->lastBound[i]), 2));

      denominator -= denomValues[i];
      denominator += numerator;

      weight = (numerator / denominator);
      if(weight > 1.0) {
         weight = 1.0;
      }

      //printf("c%d num: %.4f denom: %.4f w+: %.4f\n", i, numerator,
      //        denominator, weight);
      // printf("   ob delta: %.4f as deltamin: %.4f\n",
      //        ExampleDistance(e, centroid),
      //         AssignmentScaledDeltaMin(e, centroid, cMin, cMax));

      is->wPlus[i] += (numerator / denominator);
      is->wPlusSquare[i] += (numerator / denominator) *
                               (numerator / denominator);
      for(j = 0 ; j < ExampleSpecGetNumAttributes(es) ; j++) {
         if(ExampleGetContinuousAttributeValue(e, j) >= 0) {
            is->wxPlus[i][j] += (numerator / denominator) * 
               ExampleGetContinuousAttributeValue(e, j);
         } else {
            is->wxMinus[i][j] += (numerator / denominator) * 
               ExampleGetContinuousAttributeValue(e, j);
         }
      }

      denominator += denomValues[i];
      denominator -= numerator;
   }

   /* do the W-Minuses */
   denominator = 0;
   for(i = 0 ; i < VALLength(is->centroids) ; i++) {
      centroid = VLIndex(is->centroids, i);
      cMax = VLIndex(is->cMax, i);
      cMin = VLIndex(is->cMin, i);

      denomValues[i] = exp( (-1.0 / (2.0 * gSigmaSquare)) *
        pow(AssignmentScaledDeltaMin(e, centroid, cMin, cMax, 
                                is->lastBound[i]), 2));
      denominator += denomValues[i];
   }
   for(i = 0 ; i < VALLength(is->centroids) ; i++) {
      centroid = VLIndex(is->centroids, i);
      cMax = VLIndex(is->cMax, i);
      cMin = VLIndex(is->cMin, i);

      numerator = exp( (-1.0 / (2.0 * gSigmaSquare)) *
         pow(AssignmentScaledDeltaMax(e, centroid, cMin, cMax,
                                 is->lastBound[i]), 2));

      denominator -= denomValues[i];
      denominator += numerator;

      //printf("c%d num: %.4f denom: %.4f w-: %.4f\n", i, numerator,
      //          denominator, (numerator / denominator));
      //printf("   ob delta: %.4f as deltamax: %.4f\n",
      //          ExampleDistance(e, centroid),
      //         AssignmentScaledDeltaMax(e, centroid, cMin, cMax));

      is->wMinus[i] += (numerator / denominator);
      for(j = 0 ; j < ExampleSpecGetNumAttributes(es) ; j++) {
         if(ExampleGetContinuousAttributeValue(e, j) >= 0) {
            is->wxMinus[i][j] += (numerator / denominator) * 
                ExampleGetContinuousAttributeValue(e, j);
         } else {
            is->wxMinus[i][j] += (numerator / denominator) * 
                ExampleGetContinuousAttributeValue(e, j);
         }
      }

      denominator += denomValues[i];
      denominator -= numerator;
   }

   MFreePtr(denomValues);
}


static int _DoClusterIterationDidConverge(FILE *data, ExampleSpecPtr es, FILE *boundData) {
   int i,j;
   ExamplePtr e, centroid;

   long seen = 0;
   int done;
   IterationStatsPtr is, newIs;

   double denominator, numerator;

   is = VALIndex(gStatsList, VALLength(gStatsList) - 1);

   if(gMessageLevel > 1) {
      printf("enter iteration %d seen %d\n", gIteration, gTotalExamplesSeen);
      fflush(stdout);
   }
   done = 0;
   e = ExampleRead(data, es);
   while(e != 0 && !done ) {
      seen++;
      gTotalExamplesSeen++;
      is->n++;

      if(gMessageLevel > 3) {
         //IterationStatsWrite(is, es, stdout);
         printf("-------------------------------\nincorporating: ");
         ExampleWrite(e, stdout);
         for(i = 0 ; i < VALLength(is->centroids) ; i++) {
            printf("  c#%d: ", i);
            ExampleWrite(VALIndex(is->centroids, i), stdout);
         } 
         fflush(stdout);
      }

      /* do the Ws */
      denominator = 0;
      for(i = 0 ; i < VALLength(is->centroids) ; i++) {
         centroid = VLIndex(is->centroids, i);
         denominator += exp( (-1.0 / (2.0 * gSigmaSquare)) *
            pow(ExampleDistance(e, centroid), 2));
      }
      for(i = 0 ; i < VALLength(is->centroids) ; i++) {
         centroid = VLIndex(is->centroids, i);
         numerator = exp( (-1.0 / (2.0 * gSigmaSquare)) *
            pow(ExampleDistance(e, centroid), 2));

//if(i == 4) { printf("Denom: %.3f Numer: %.3f w: %.3f distance: %.3f, true: %d\n",
//        denominator, numerator, (numerator / denominator),
//           ExampleDistance(e, centroid), ExampleGetClass(e)); }
        

         is->w[i] += (numerator / denominator);
         for(j = 0 ; j < ExampleSpecGetNumAttributes(es) ; j++) {
            is->wx[i][j] += (numerator / denominator) * 
                 ExampleGetContinuousAttributeValue(e, j);
         }
      }

      if(gUseGeoffBound) {
         _RecordGeoffBoundInfo(e, is, es);
      }

      if(gMessageLevel > 3) {
         IterationStatsWrite(is, es, stdout);
         fflush(stdout);
      }

      ExampleFree(e);

      /* check to see if we should move to the next iteration */
      if(!gBatch && gFancyStop && gIteration <= gNumIterationNs) {
         /* Test to see if this iteration is done */
         if(seen >= gIterationNs[gIteration - 1]) {
            done = 1;
         }
      } else if(!gBatch && seen >= gN) {
         done = 1;
      } else if(seen > gMaxExamplesPerIteration) {
         done = 1;
      }

      if(!done) {
         /* if we didn't get stopped by the termination check get another */
         e = ExampleRead(data, es);
      }
   }
   
   if(gMessageLevel > 1) {
      printf("Finished an iteration, n is %ld.\n", is->n);
      IterationStatsWrite(is, es, stdout);
   }

   newIs = IterationStatsNext(is, gNeededDelta, 1.0,
                 gAssignErrorScale, es, !gUseGeoffBound, boundData);

   VALAppend(gStatsList, newIs);

   if(gMessageLevel > 1) {
      printf("exit iteration %d seen %d\n", gIteration, gTotalExamplesSeen);
      fflush(stdout);
   }
   
   if(newIs) {
      return _CheckConverganceUpdateStats(is, newIs);
   } else {
      /* we didn't converge, but this round will be stoped by
           the foundBound of 0 */
      return 0;
   }
}

static ExamplePtr _PickInitalCentroid(ExampleSpecPtr es, VoidAListPtr centroids, FILE *data) {
   float minDistance;
   int done  = 0;
   ExamplePtr e;
   int used;
   int i;

   minDistance = gR / ((float)gNumClusters * 4);
   //minDistance = gR / ((float)gNumClusters * 2);

   while(!done) {
      e = ExampleRead(data, es);
      DebugError(e == 0, "Unable to get enough unique initial centroids");

      /* make sure this isn't too close to one in the list */
      used = 0;
      for(i = 0 ; i < VALLength(centroids) && !used ; i++) {
         /* HERE make a parameter? */
         if(ExampleDistance(e, VALIndex(centroids, i)) <= minDistance) {
            used = 1;
         }
      }

      /* if it's ok then use it */
     if(!used) {
         done = 1;
      } else {
         ExampleFree(e);
      }
   }
   return e;
}

static void _PickInitialCentroids(ExampleSpecPtr es, VoidAListPtr centroids,
                                       FILE *data) {
   /* pick the first unique 'gNumClusters' points from the dataset to
            be centroids  HERE should I add some randomness to this? */
   int j;
   ExamplePtr e;
   char fileNames[255];
   FILE *centersIn;

   /* burn some examples to make the selection be more random */
   for(j = RandomRange(0, 10 * gNumClusters) ; j > 0 ; j--) {
      ExampleFree(ExampleRead(data, es));
   }

   /* if instructed, read in the initial centroids */
   if(gLoadCenters) {
      sprintf(fileNames, "%s.centers", gFileStem);
      centersIn = fopen(fileNames, "r");

      if(centersIn) {
         if(gMessageLevel > 0) {
            printf("Loading inital centers from %s\n", fileNames);
         }

         e = ExampleRead(centersIn, es);
         while(e != 0) {
            VALAppend(centroids, e);
            e = ExampleRead(centersIn, es);

         }
         fclose(centersIn);
      }
   }

   while(VALLength(centroids) < gNumClusters) {
      VALAppend(centroids, _PickInitalCentroid(es, centroids, data));
   }

   if(gMessageLevel > 0) {
      printf("loss for initial centroids:\n");
      _doTests(es, centroids, 0, 0, 0);
   }
}

//static void _OutputGoodCentroids(float bound) {
//   int i;
//   char fileNames[255];
//   FILE *centersOut;
//   IterationStatsPtr thisIs, firstIs;
//   float totalError, errorThreshold;

   /* write out the centroids, if we didn't get a bound only write out the
        good ones, where lastBound < (100 / gNumClusers)% of the total
         lastBound error */

//   thisIs = VALIndex(gStatsList, VALLength(gStatsList) - 1);
//   firstIs = VALIndex(gStatsList, 0);

//   sprintf(fileNames, "%s.centers", gFileStem);
//   centersOut = fopen(fileNames, "w");

//   totalError = 0;
//   for(i = 0 ; i < VALLength(thisIs->centroids) ; i++) {
//      totalError += thisIs->lastBound[i];
//   }

//   errorThreshold = totalError / (float)gNumClusters;
   //IterationStatsWrite(thisIs, stdout);

//   if(gMessageLevel > 0) {
//      printf("output good centers, thresh: %f\n", errorThreshold);
//   }

//   for(i = 0 ; i < VALLength(thisIs->centroids) ; i++) {
//      /* if we finished with a bound or if the centroid's error was ok */
//      if((thisIs->guarenteeIDConverge && thisIs->foundBound &&
//                                  bound <= gThisErrorTarget) || 
//           (thisIs->lastBound[i] <  errorThreshold)) {
//         ExampleWrite(VALIndex(firstIs->centroids, i), centersOut);
//      }
//   }
//   fclose(centersOut);
//}

static void _OutputAllCentroids(float bound) {
   int i;
   char fileNames[255];
   FILE *centersOut;
   IterationStatsPtr thisIs;

   thisIs = VALIndex(gStatsList, VALLength(gStatsList) - 1);

   sprintf(fileNames, "%s.centers", gFileStem);
   centersOut = fopen(fileNames, "w");

   for(i = 0 ; i < VALLength(thisIs->centroids) ; i++) {
      ExampleWrite(VALIndex(thisIs->centroids, i), centersOut);
   }
   fclose(centersOut);
}

static void _OutputCentroidMovement(void) {
   IterationStatsPtr firstIs, lastIs;
   int i;
   float total, current, totalSquare;

   firstIs = VALIndex(gStatsList, 0);
   lastIs = VALIndex(gStatsList, VALLength(gStatsList) - 1);

   total = 0;
   totalSquare = 0;
   for(i = 0 ; i < VALLength(firstIs->centroids) ; i++) {
      current = ExampleDistance(VALIndex(firstIs->centroids, i),
                             VALIndex(lastIs->centroids, i));

      total += current;
      totalSquare += pow(current, 2);

      printf(" centroid %d moved: %f move^2: %f\n", i, current, pow(current, 2));
   }
   printf("   total: %f total^2: %f\n", total, totalSquare);
}

static IterationStatsPtr _FindLastIsWithBound(void) {
   int i;
   IterationStatsPtr is;

   for(i = VALLength(gStatsList) - 1 ; i >= 0 ; i--) {
      is = VALIndex(gStatsList, i);
      if(is->foundBound) {
         return is;
      }
   }

   DebugWarn(1, "_FindLastIsWithBound didn't find a bound\n");
   return 0;
}

static float _FindMedian(float *array, int len) {
   float *errorArray = MNewPtr(sizeof(float) * len);
   float tmp, median;
   int i, j;

   /* create the sorted error array which we'll need to find the median */
   for(i = 0 ; i < len ; i++) {
      errorArray[i] = array[i];
   }
   for(i = 0 ; i < len ; i++) {
      for(j = 0 ; j < len - (i + 1) ; j++) {
         if(errorArray[j] > errorArray[j + 1]) {
            tmp = errorArray[j + 1];
            errorArray[j + 1] = errorArray[j];
            errorArray[j] = tmp;
         }
      }
   }

   if(len % 2 == 0) {
      i = (len / 2) - 1;
      median = (errorArray[i] + errorArray[i + 1]) / 2.0;
   } else {
      i = ((len + 1) / 2) - 1;
      median = errorArray[i];
   }
   MFreePtr(errorArray);

   return median;
}

VoidAListPtr _GetCentroidsForNextRound(FILE *data, ExampleSpecPtr es) {
   IterationStatsPtr is = _FindLastIsWithBound();
//   IterationStatsPtr is = VALIndex(gStatsList, VALLength(gStatsList) - 1);
   float median = _FindMedian(is->lastBound, VALLength(is->centroids));
   IterationStatsPtr iIs = VALIndex(gStatsList, 0);
   VoidAListPtr newCentroids = VALNew();
   int i;


   //IterationStatsWrite(is, stdout);

   for(i = 0 ; i < VALLength(is->centroids) ; i++) {
      if(is->lastBound[i] <= median * 5) {
         VALAppend(newCentroids, ExampleClone(VALIndex(iIs->centroids, i)));
         //VALAppend(newCentroids, ExampleClone(VALIndex(is->centroids, i)));
      } else if(gMessageLevel > 1) {
         printf("   Reassigning centroid %d bound %f median %f\n", 
                       i, is->lastBound[i], median);
         fflush(stdout);
      }
   }

   while(VALLength(newCentroids) < VALLength(is->centroids)) {
      VALAppend(newCentroids, _PickInitalCentroid(es, newCentroids, data));
   }

   return newCentroids;
}

/* this should be static */
void CalculateExamplesPerIteration(VoidAListPtr last, float **nextNiOut, int *num);

int main(int argc, char *argv[]) {
   char fileNames[255];

   FILE *exampleIn = 0, *boundDataIn = 0;
   ExampleSpecPtr es;
   ExamplePtr e;
   VoidListPtr centers, newCenters = 0;
   float iterationNSum;

   long learnTime;
   int i;
   int breakOut;
   int fileDone;
   long nIncrement;
   float lastDelta, bound;

   struct tms starttime;
   struct tms endtime;

   IterationStatsPtr thisIs, lastIs;

   _processArgs(argc, argv);


   if(gStdin) {
      /* This is a hack because when I pipe clusterdata to vfem
           vfem tries to read the spec before clusterdata can write it */
      sleep(5);
   }

   sprintf(fileNames, "%s/%s.names", gSourceDirectory, gFileStem);
   es = ExampleSpecRead(fileNames);
   DebugError(es == 0, "Unable to open the .names file");

   
   RandomInit();
   /* seed for the concept */
   if(gSeed != -1) {
      RandomSeed(gSeed);
   } else {
      gSeed = RandomRange(1, 30000);
      RandomSeed(gSeed);
   }
   if(gMessageLevel > 0) {
      printf("running with seed %d\n", gSeed);
   }

   /* initialize some globals */
   gStatsList = VALNew();
   gD = ExampleSpecGetNumAttributes(es);

   if(gR == 0) {
      gR = sqrt(gD);
   }

   if(!gAllowBadConverge) {
      /* use a tighter bound so we get a better convergence behavior */
      gThisErrorTarget = min(gErrorTarget, gConvergeDelta / 3.0);
   } else {
      /* HERE this is a hack for now, take it out!! */
      gThisErrorTarget = gErrorTarget;
      //gThisErrorTarget = min(gErrorTarget, gConvergeDelta);
   }
   gNeededDelta = 1.0 - pow(1.0 - gDelta, 1.0 /
                 (float)(gD * gNumClusters * gEstimatedNumIterations));
   gTargetEkd = sqrt(gThisErrorTarget / ((float)gNumClusters * (float)gD));

   /* HERE fix this for the new bound */
   gN = (gNumClusters / 2.0) * pow(1.0/gTargetEkd, 2) * 
                          log(2.0/gNeededDelta) * 1.1;

   if(gMessageLevel > 1) {
      printf("Target Ekd: %.3lf\n", gTargetEkd);
   }

   if(gStdin) {
      exampleIn = stdin;
      if(!gUseGeoffBound) {
         if(gMessageLevel > 0) {
            printf("Trying to use the tight bound on stdin won't work, reverting to the looser one pass bound.\n");
         }
         gUseGeoffBound = 1;
      }
   }

   /* Pick the initial centroids from the dataset */
   if(!gStdin) {
      /* in stream mode we never close & reopen the file */
      sprintf(fileNames, "%s/%s.data", gSourceDirectory, gFileStem);
      exampleIn = fopen(fileNames, "r");
      DebugError(exampleIn == 0, "Unable to open the .data file");
   }

   centers = VALNew();
   /* try get the gInitNumber-ith batch of centroids */
   for(i = 0 ; i < gInitNumber ; i++) {
      while(VALLength(centers) > 0) {
         ExampleFree(VALRemove(centers, VALLength(centers) - 1));
      }
      _PickInitialCentroids(es, centers, exampleIn);
   }

   if(!gStdin) {
      /* in stream mode we never close & reopen the file */
      fclose(exampleIn);
   }

   /* create the initial stats structure from the initial centroids */
   VALAppend(gStatsList, IterationStatsInitial(centers));

   if(gMessageLevel >= 1) {
      printf("allocation %ld\n", MGetTotalAllocation());
   }
   learnTime = 0;

   gIteration = 0;
   gRound = 0;

   times(&starttime);
   do {
      gRound++;

      /* if we aren't on the first round free the cruft from the prvious one */
      if(gRound > 1) {

         if(gReassignCentersEachRound) {
            if(!gStdin) {
               /* in stream mode we never close & reopen the file */
               sprintf(fileNames, "%s/%s.data", gSourceDirectory, gFileStem);
               exampleIn = fopen(fileNames, "r");
               DebugError(exampleIn == 0, 
                                   "Unable to open the .data file");
            }

            /* burn some examples to make the selection be more random */
            for(i = RandomRange(0, 10 * gNumClusters) ; i > 0 ; i--) {
               ExampleFree(ExampleRead(exampleIn, es));
            }

            /* HERE MEM this newCenters may leak memory */
            newCenters = _GetCentroidsForNextRound(exampleIn, es);
         }

         for(i = VALLength(gStatsList) - 1 ; i >= 0 ; i--) {
            IterationStatsFree(VALRemove(gStatsList, i));
         }
         if(gReassignCentersEachRound) {
            VALAppend(gStatsList, IterationStatsInitial(newCenters));
         } else {
            /* use the inital centroids again */
            VALAppend(gStatsList, IterationStatsInitial(centers));
         }
      }

      gIteration = 1;

      if(!gStdin) {
         /* in stream mode we never close & reopen the file */
         sprintf(fileNames, "%s/%s.data", gSourceDirectory, gFileStem);
         exampleIn = fopen(fileNames, "r");
         boundDataIn = fopen(fileNames, "r");
         DebugError(exampleIn == 0 || boundDataIn == 0,
                              "Unable to open the .data file");
      }

      if(gMessageLevel > 0) {
         printf("========================\n");
         printf("round %d n %ld\n", gRound, gN);
      }

      while(!_DoClusterIterationDidConverge(exampleIn, es, boundDataIn)) {
         times(&endtime);

         /* see if we are toast, if so don't do the rest of the book keep */
         thisIs = VALIndex(gStatsList, VALLength(gStatsList) - 1);
         breakOut = 0;
         lastIs = 0;
         if(!gBatch &&
                (_CalculateIDKMEarlyBound() > (40000 * gThisErrorTarget) ||
                !thisIs->foundBound)) {
            /* do one last check to see if we're on the last round */
            if(VALLength(gStatsList) > 1) {
               lastIs = VALIndex(gStatsList, VALLength(gStatsList) - 2);
               if(!(lastIs->n >= gDBSize)) {
                  breakOut = 1;
               } else {
                  breakOut = 0;
               }
            } else {
               breakOut = 0;
            }
         }

         if(breakOut) {
            /* IDEM could have stoped and we are a loooong way away */
            if(gMessageLevel > 1) {
               printf("      broke out of a round early foundBound %d current bound %f - Target * 40000: %f\n", thisIs->foundBound,
                    _CalculateIDKMEarlyBound(), 40000 * gThisErrorTarget);
               printf("        dbSize: %ld n: %ld\n", gDBSize, lastIs->n);
            }
            thisIs->foundBound = 0;
            break;
         }

         /* do the book keeping to go on to the next iteration */
         learnTime += endtime.tms_utime - starttime.tms_utime;

         /* reset the file for the next iteration */
         if(!gStdin) {
            /* in stream mode we never close & reopen the file */
            fclose(exampleIn);
            fclose(boundDataIn);
            sprintf(fileNames, "%s/%s.data", gSourceDirectory, gFileStem);
            exampleIn = fopen(fileNames, "r");
            boundDataIn = fopen(fileNames, "r");
            DebugError(exampleIn == 0 || boundDataIn == 0,
                                 "Unable to open the .data file");
         }

         if(gDoTests) {
            _doTests(es, thisIs->centroids, gIteration, learnTime, 0);
         }

         gIteration++;

         /* reset the timer for the next iteration */
         times(&starttime);
      } /* end of a round */


      /* test to see if we can get anything else from the file, if not
            then we won't be able to increase n and we are finished */
      if(!gStdin) {
         e = ExampleRead(exampleIn, es);
         if(e == 0) {
            fileDone = 1;
         } else {
            fileDone = 0;
            ExampleFree(e);
         }

         /* close the file for the beginning of the next round can open */
         fclose(exampleIn);
         fclose(boundDataIn);
      } else {
         if(gN >= gMaxExamplesPerIteration) {
            fileDone = 1;
         } else {
            fileDone = 0;
         }
      }

      thisIs = VALIndex(gStatsList, VALLength(gStatsList) - 1);

      if(thisIs->foundBound && thisIs->guarenteeIDConverge) {
         /* re-estimate l, update effective delta */
         gEstimatedNumIterations = gIteration * 1.5;

         lastDelta = gNeededDelta;
         gNeededDelta = 1.0 - pow(1.0 - gDelta, 1.0 /
                 (float)(gD * gNumClusters * gEstimatedNumIterations));

         /* HERE fix this */
         nIncrement = pow(thisIs->maxEkd/gTargetEkd, 2) *
                 (log(2.0 / gNeededDelta) / log(2.0 / lastDelta)) *
                      gN * 1.1;

         if(gMessageLevel > 1) {
            printf("suggested gN increment: %ld current gN: %ld\n", nIncrement, gN);
         }

         if(nIncrement > gN) {
            gN += nIncrement;
         } else {
            gN *= 2;
         }
         if(gN > gMaxExamplesPerIteration) {
            gN = gMaxExamplesPerIteration;
         }
      } else {
         gN *= 2;
      }

      if(gIterationNs != 0) {
         MFreePtr(gIterationNs);
         gIterationNs = 0;
         gNumIterationNs = 0;
      }
      if(!gBatch && gFancyStop && thisIs->foundBound) {
         if(gMessageLevel > 2) { IterationStatsWrite(thisIs, es, stdout); }

         CalculateExamplesPerIteration(gStatsList, &gIterationNs,
                         &gNumIterationNs);

         /* normalize & update to # examples per iteration */
         iterationNSum = 0;
         for(i = 0 ; i < gNumIterationNs ; i++) {
            iterationNSum += gIterationNs[i];
         }

         if(iterationNSum > gDBSize * gNumIterationNs) {
            /* go on to a final round */
            if(gMessageLevel > 2) {
               printf("CalculateExamplesPerIteration says we need too many samples, just go to one final round with the whole DB.\n");
            }
            gN = gDBSize + 1; /* make sure it's final */
            if(gIterationNs != 0) {
               MFreePtr(gIterationNs);
               gIterationNs = 0;
               gNumIterationNs = 0;
            }
         } else {
            if(iterationNSum < gN * gNumIterationNs) {
               /* use the actual Nis only if their sum is larger than 
                    the number of examples suggested by gN,
                    otherwise use their ratios as the percent of gN * l
                    to use in each iteration */
               for(i = 0 ; i < gNumIterationNs ; i++) {
                  gIterationNs[i] = ((gIterationNs[i] / iterationNSum) * gN) *
                          gNumIterationNs;
               }
            }

            if(gMessageLevel > 1) {
               printf("ni:\n");
               for(i = 0 ; i < gNumIterationNs ; i++) {
                  printf("\t%d: %.4f\n", i, gIterationNs[i]);
               }
            }
         }
      }

      bound = _CalculateErrorBound();
      if(gMessageLevel > 1) {
         printf("   found bound %d bound: %f guarentee converge %d file done %d\n", thisIs->foundBound, bound, thisIs->guarenteeIDConverge, fileDone);
         printf("===");
         fflush(stdout);
      }

   } while(!(thisIs->guarenteeIDConverge && thisIs->foundBound && 
                bound <= gThisErrorTarget) && 
           !(gAllowBadConverge && thisIs->wouldEMConverge && 
                thisIs->foundBound && bound <= gThisErrorTarget) &&
            !fileDone && !gBatch);



   times(&endtime);
   learnTime += endtime.tms_utime - starttime.tms_utime;

   _OutputAllCentroids(bound);

   if(gMessageLevel > 0) {
      _OutputCentroidMovement();
   }

   _doTests(es, 
      ((IterationStatsPtr)VALIndex(gStatsList,
             VALLength(gStatsList) - 1))->centroids,
                   gIteration, learnTime, 0);

   fclose(stdin);
   return 0;
}


/* God help you if you need to comprehend or change this function */
/*  Get our paper and read it */
void CalculateExamplesPerIteration(VoidAListPtr last, float **nextNiOut, int *num) {
   IterationStatsPtr lastIs, currentIs;
   float **ni, **a, **b, **r;
   float tmp, thisDelta;
   int i, j, k;

   *num = VALLength(last) - 1;
   (*nextNiOut) = MNewPtr(sizeof(float) * *num);

   thisDelta = 1.0 - pow(1.0 - gDelta, 1.0 / 
                 (float)(gD * gNumClusters * *num));

   ni     = MNewPtr(sizeof(float *) * gNumClusters);
   a      = MNewPtr(sizeof(float *) * gNumClusters);
   b      = MNewPtr(sizeof(float *) * gNumClusters);
   r      = MNewPtr(sizeof(float *) * gNumClusters);

   for(i = 0 ; i < gNumClusters ; i++) {
      ni[i] = MNewPtr(sizeof(float) * *num);
      a[i] = MNewPtr(sizeof(float) * *num);
      b[i] = MNewPtr(sizeof(float) * *num);
      r[i] = MNewPtr(sizeof(float) * *num);
   }

   for(i = 0 ; i < *num ; i++) {
      lastIs = VALIndex(last, i);
      for(k = 0 ; k < gNumClusters ; k++) {
         currentIs = VALIndex(last, i + 1);
         if(i == 0) {
            a[k][i] = 0;
         } else {
            a[k][i] = currentIs->lastAssignmentBound[k] / 
                     lastIs->lastBound[k];
         }

         b[k][i] = pow(lastIs->wMinus[k], 2) / 
                 ((float)lastIs->n * lastIs->wPlusSquare[k]);
      }
   }

   for(i = 0 ; i < *num ; i++) {
      for(k = 0 ; k < gNumClusters ; k++) {
         /* Here is this the right place for this gD?*/
         r[k][i] = sqrt((gD * log(2)) / (2.0 * b[k][i]));
         for(j = i + 1 ; j < *num ; j++) {
            r[k][i] *= a[k][j];
         }
      }
   }

   for(i = 0 ; i < *num ; i++) {
      for(k = 0 ; k < gNumClusters ; k++) {
         tmp = 0;
         for(j = 0 ; j < gNumClusters ; j++) {
            tmp += pow((r[k][i] * pow(r[k][j], 2)) , 1.0 / 3.0);
         }

         ni[k][i] = ((float)gNumClusters / gThisErrorTarget);
         ni[k][i] *= pow(tmp, 2);
      }
   }


   for(i = 0 ; i < *num ; i++) {
      (*nextNiOut)[i] = 0;

      for(k = 0 ; k < gNumClusters ; k++) {
         if(ni[k][i] > (*nextNiOut)[i]) {
            (*nextNiOut)[i] = ni[k][i];
         }
      }
   }

   if(gMessageLevel > 2) {
      for(i = 0 ; i < *num ; i++) {
         printf("iteration %d nextTotal: %f\n", i, (*nextNiOut)[i]);
         for(k = 0 ; k < gNumClusters ; k++) {
            printf("  c%d: ", k);
            printf("a %.3f b %.3f r %.3f ", a[k][i], b[k][i], r[k][i]);
            printf("ni %.3f", ni[k][i]);
            printf("\n");
         }
      }
   }

   /* Free memory */
   for(i = 0 ; i < gNumClusters ; i++) {
      MFreePtr(ni[i]);
      MFreePtr(a[i]);
      MFreePtr(b[i]);
      MFreePtr(r[i]);
   }
   MFreePtr(ni);
   MFreePtr(a);
   MFreePtr(b);
   MFreePtr(r);
}

