#include "vfem-engine.h"
#include <math.h>
#include <stdlib.h>

extern float gNeededDelta;
extern float gSigmaSquare;
extern int gNormalApprox;

IterationStatsPtr IterationStatsInitial(VoidAListPtr initialClusters) {
   IterationStatsPtr is;
   int i;

   is = MNewPtr(sizeof(IterationStats));
   is->centroids = VLNew();
   for(i = 0 ; i < VALLength(initialClusters) ; i++) {
      VALAppend(is->centroids, ExampleClone(VALIndex(initialClusters, i)));
   }

   is->cMax = VLNew();
   for(i = 0 ; i < VALLength(initialClusters) ; i++) {
      VALAppend(is->cMax, ExampleClone(VALIndex(initialClusters, i)));
   }

   is->cMaxAssignment = VLNew();
   for(i = 0 ; i < VALLength(initialClusters) ; i++) {
      VALAppend(is->cMaxAssignment,
                 ExampleClone(VALIndex(initialClusters, i)));
   }

   is->cMin = VLNew();
   for(i = 0 ; i < VALLength(initialClusters) ; i++) {
      VALAppend(is->cMin, ExampleClone(VALIndex(initialClusters, i)));
   }

   is->cMinAssignment = VLNew();
   for(i = 0 ; i < VALLength(initialClusters) ; i++) {
      VALAppend(is->cMinAssignment,
                 ExampleClone(VALIndex(initialClusters, i)));
   }
   is->lastBound = MNewPtr(sizeof(float) * VALLength(is->centroids));
   is->lastAssignmentBound = MNewPtr(sizeof(float) * VALLength(is->centroids));

   is->w = MNewPtr(sizeof(double) * VALLength(is->centroids));
   is->wPlus = MNewPtr(sizeof(double) * VALLength(is->centroids));
   is->wPlusSquare = MNewPtr(sizeof(double) * VALLength(is->centroids));
   is->wMinus = MNewPtr(sizeof(double) * VALLength(is->centroids));


   is->wx = MNewPtr(sizeof(double *) * VALLength(is->centroids));
   is->wxPlus = MNewPtr(sizeof(double *) * VALLength(is->centroids));
   is->wxMinus = MNewPtr(sizeof(double *) * VALLength(is->centroids));

   is->deltaPlus = MNewPtr(sizeof(double *) * VALLength(is->centroids));
   is->deltaMinus = MNewPtr(sizeof(double *) * VALLength(is->centroids));

   for(i = 0 ; i < VALLength(initialClusters) ; i++) {
      is->wx[i] = MNewPtr(sizeof(double) * 
          ExampleGetNumAttributes(VALIndex(is->centroids, 0)));
      is->wxPlus[i] = MNewPtr(sizeof(double) * 
          ExampleGetNumAttributes(VALIndex(is->centroids, 0)));
      is->wxMinus[i] = MNewPtr(sizeof(double) * 
          ExampleGetNumAttributes(VALIndex(is->centroids, 0)));
      is->deltaPlus[i] = MNewPtr(sizeof(double) * 
          ExampleGetNumAttributes(VALIndex(is->centroids, 0)));
      is->deltaMinus[i] = MNewPtr(sizeof(double) * 
          ExampleGetNumAttributes(VALIndex(is->centroids, 0)));
   }

   IterationStatsClearCounts(is);

   /* we always have a perfect bound for an initial is */
   is->foundBound = 1;
   is->isFirstIteration = 1;

   return is;
}

void IterationStatsClearCounts(IterationStatsPtr is) {
   int i, j;

   is->n = 0;

   for(i = 0 ; i < VALLength(is->centroids) ; i++) {
      is->lastBound[i] = 0;
      is->lastAssignmentBound[i] = 0;
      is->w[i] = 0;
      is->wPlus[i] = 0;
      is->wPlusSquare[i] = 0;
      is->wMinus[i] = 0;

      for(j = 0 ; j < ExampleGetNumAttributes(VALIndex(is->centroids, 0)) ; j++) {
         is->wx[i][j] = 0;
         is->wxPlus[i][j] = 0;
         is->wxMinus[i][j] = 0;
         is->deltaPlus[i][j] = 0;
         is->deltaMinus[i][j] = 0;
      }
   }

   is->maxEkd = 0;
   is->possibleIDConverge = 0;
   is->guarenteeIDConverge = 0;
   is->wouldEMConverge = 0;
   is->convergeVFEM = 0;
}

void IterationStatsFree(IterationStatsPtr is) {
   int numCentroids = VALLength(is->centroids);
   int i;
   ExamplePtr e;

   for(i = 0 ; i < numCentroids ; i++) {
      e = VALIndex(is->centroids, i);
      ExampleFree(e);
      e = VALIndex(is->cMin, i);
      ExampleFree(e);
      e = VALIndex(is->cMinAssignment, i);
      ExampleFree(e);
      e = VALIndex(is->cMax, i);
      ExampleFree(e);
      e = VALIndex(is->cMaxAssignment, i);
      ExampleFree(e);
   }
   VALFree(is->centroids);
   VALFree(is->cMinAssignment);
   VALFree(is->cMaxAssignment);
   VALFree(is->cMin);
   VALFree(is->cMax);

   MFreePtr(is->lastBound);
   MFreePtr(is->lastAssignmentBound);

   for(i = 0 ; i < VALLength(is->cMin) ; i++) {
      MFreePtr(is->wx[i]);
      MFreePtr(is->wxPlus[i]);
      MFreePtr(is->wxMinus[i]);
      MFreePtr(is->deltaMinus[i]);
      MFreePtr(is->deltaPlus[i]);
   }

   MFreePtr(is->w);
   MFreePtr(is->wx);
   MFreePtr(is->wPlus);
   MFreePtr(is->wPlusSquare);
   MFreePtr(is->wMinus);
   MFreePtr(is->wxPlus);
   MFreePtr(is->wxMinus);
   MFreePtr(is->deltaMinus);
   MFreePtr(is->deltaPlus);

   MFreePtr(is);
}

/* this is so terrible */
static void _MakeNewCentroids(VoidAListPtr centers, VoidAListPtr newCenters,
    VoidAListPtr newAssignment, double **wx, double *w, double *wPlusSquare,
    double *wMinus, long n, ExampleSpecPtr es, float delta, int useSampleError,
       int boundUpper, int pedroBound, double **deltaPlus, 
              double **deltaMinus) {
   double epsilon, *thisWx;
   ExamplePtr newCenter, newCenterAssignment = 0, e;
   int i,j;
   double assignmentError;

   /* free any old centers from the list */
   while(VALLength(newCenters) > 0) {
      i = VALLength(newCenters) - 1;
      e = VALRemove(newCenters, i);
      ExampleFree(e);
   }

   if(useSampleError) {
      while(VALLength(newAssignment) > 0) {
         i = VALLength(newAssignment) - 1;
         e = VALRemove(newAssignment, i);
         ExampleFree(e);
      }
   }

   // This is my bound
   //if(useBound) {
   //   epsilon = ((double)n) * log(2.0 / delta);
   //   epsilon /= 2.0;
   //   epsilon = sqrt(epsilon);
   //} else {
   //   epsilon = 0;
   //}

   if(gMessageLevel > 2) {
      printf("Making new centroids.\n");
      fflush(stdout);
   }

   /* make the list of new centers */
   for(i = 0 ; i < VALLength(centers) ; i++) {
      if(useSampleError) {
         newCenterAssignment = ExampleNew(es);
         ExampleSetClass(newCenterAssignment, 0);
      }

      newCenter = ExampleNew(es);
      ExampleSetClass(newCenter, 0);

      if(useSampleError) {
         epsilon = wPlusSquare[i] / (wMinus[i] * wMinus[i]);
         if(epsilon > 1) {
            epsilon = 1;
         } else if(epsilon < (1.0 / n)) {
            epsilon = 1.0 / n;
         }

         epsilon *= log(2.0 / delta);
         epsilon /= 2.0;
         epsilon = sqrt(epsilon);

         if(gNormalApprox) {
            epsilon /= 50;
         }

         //printf("i %d j %d\tw/w %lf epsilon %lf\n", 
         //i, j, thisWSquare[j] / (thisW[j] * thisW[j]), epsilon);
      } else {
         epsilon = 0;
      }

      thisWx = wx[i];

      for(j = 0 ; j < ExampleSpecGetNumAttributes(es) ; j++) {

         if(pedroBound) {
            assignmentError = max(deltaPlus[i][j], deltaMinus[i][j]) / 
                         wMinus[i];
            if(gMessageLevel > 2) {
               printf("   assignment bound cluster %d dim %d: %.2f\n", i, j,
                        assignmentError);
            }
         } else {
            assignmentError = 0;
         }

         if(ExampleSpecIsAttributeContinuous(es, j)) {
            if(!(w[i] <= 0 || (boundUpper && (w[i] - epsilon <= 0)))) {
               if(boundUpper) {
                  ExampleSetContinuousAttributeValue(newCenter, j,
                      (thisWx[j] / w[i]) + epsilon + assignmentError);
               } else { /* bound lower */
                  /* HERE remove this min at some point */
                  ExampleSetContinuousAttributeValue(newCenter, j,
                      max((thisWx[j] / w[i]) - (epsilon + assignmentError),
                           0));
               }

               if(useSampleError) {
                  /* make the assignment error only center */
                  if(boundUpper) {
                     ExampleSetContinuousAttributeValue(newCenterAssignment, j,
                         (thisWx[j] / w[i]) + assignmentError);
                  } else {
                  /* HERE remove this min at some point */
                     ExampleSetContinuousAttributeValue(newCenterAssignment, j,
                         max((thisWx[j] / w[i]) - assignmentError, 0));
                  }
               }
            } else {
               /* not enough weight so leave the center the same */
               if(gMessageLevel > 1) {
                  printf("not enough weight: %f epsilon: %f, possible div by zero c: %d a: %d!\n",
                                w[i], epsilon, i,j);
               }
               ExampleSetContinuousAttributeValue(newCenter, j,
                 ExampleGetContinuousAttributeValue(VALIndex(centers, i),j));
            }
         } else {
            /* HERE what to do about discrete attributes */
         }
      }

      VALAppend(newCenters, newCenter);
      if(useSampleError) {
         VALAppend(newAssignment, newCenterAssignment);
      }
   }
}

float AssignmentScaledDeltaMax(ExamplePtr e,
                          ExamplePtr centroid, ExamplePtr min, 
                           ExamplePtr max, float epsilon);
float AssignmentScaledDeltaMin(ExamplePtr e,
                          ExamplePtr centroid, ExamplePtr min, 
                           ExamplePtr max, float epsilon);

static void _RecordPedroBoundInfo(FILE *boundData, IterationStatsPtr is, VoidAListPtr newCentroids, ExampleSpecPtr es) {
   int i, j;
   long n;
   ExamplePtr centroid, newCentroid, cMin, cMax, e;
   double denominator, numerator, delta, weight;
   double *denomValues;


   denomValues = MNewPtr(sizeof(double) * VALLength(is->centroids));

   DebugWarn(VALLength(is->centroids) == 0,
                  "newCentroid will be used uninitialized\n");
   newCentroid = 0;


   e = ExampleRead(boundData, es);
   n = 1;
   while(e != 0 && n <= is->n) {
      if(gMessageLevel > 4) {
         ExampleWrite(e, stdout);
      }

      /* HERE modify for negative Xs */
      /* do the W-Plusses */
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
         newCentroid = VALIndex(newCentroids, i);
         cMax = VLIndex(is->cMax, i);
         cMin = VLIndex(is->cMin, i);

         numerator = exp( (-1.0 / (2.0 * gSigmaSquare)) *
            pow(AssignmentScaledDeltaMin(e, centroid, cMin, cMax,
                             is->lastBound[i]), 2));

         denominator -= denomValues[i];
         denominator += numerator;

         weight = (numerator /denominator);
         if(weight > 1.0) {
            weight = 1.0;
         }

         //printf("c%d num: %.4f denom: %.4f w+: %.4f\n", i, numerator,
         //        denominator, weight);
         //printf("   ob delta: %.4f as deltamin: %.4f\n",
         //        ExampleDistance(e, centroid),
         //         AssignmentScaledDeltaMin(e, centroid, cMin, cMax));

         is->wPlus[i] += weight;
         is->wPlusSquare[i] += pow(weight, 2);
         for(j = 0 ; j < ExampleSpecGetNumAttributes(es) ; j++) {
            is->wxPlus[i][j] += weight * 
                        ExampleGetContinuousAttributeValue(e, j);
 
            delta = ExampleGetContinuousAttributeValue(e, j) -
                    ExampleGetContinuousAttributeValue(newCentroid, j);
            if(delta > 0) {
               is->deltaPlus[i][j] += weight * delta;
            } else {
               is->deltaMinus[i][j] += weight * delta;
            }
            if(gMessageLevel > 4) {
               printf("w+ %.2f D %.2f D+ %.2f D- %.2f\n", 
                  weight, delta, is->deltaPlus[i][j], is->deltaMinus[i][j]);
            }
         }
         denominator += denomValues[i];
         denominator -= numerator;
      }

      /* HERE should denominator use min for the centroid in numerator? */
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

         weight = (numerator /denominator);

         //printf("c%d num: %.4f denom: %.4f w-: %.4f\n", i, numerator,
         //          denominator, (numerator / denominator));
         //printf("   ob delta: %.4f as deltamax: %.4f\n",
         //          ExampleDistance(e, centroid),
         //         AssignmentScaledDeltaMax(e, centroid, cMin, cMax));

         is->wMinus[i] += weight;
         for(j = 0 ; j < ExampleSpecGetNumAttributes(es) ; j++) {
            is->wxMinus[i][j] += weight * 
                ExampleGetContinuousAttributeValue(e, j);

            delta = ExampleGetContinuousAttributeValue(e, j) -
                    ExampleGetContinuousAttributeValue(newCentroid, j);
            if(delta > 0) {
               is->deltaMinus[i][j] += weight * delta;
            } else {
               is->deltaPlus[i][j] += weight * delta;
            }
            if(gMessageLevel > 4) {
               printf("w- %.2f D %.2f D+ %.2f D- %.2f\n", 
                  weight, delta, is->deltaPlus[i][j], is->deltaMinus[i][j]);
            }
         }
         denominator += denomValues[i];
         denominator -= numerator;
      }

      ExampleFree(e);
      e = ExampleRead(boundData, es);
      n++;
   }

   if(e != 0) {
      ExampleFree(e);
   }

   for(i = 0 ; i < VALLength(is->centroids) ; i++) {
      for(j = 0 ; j < ExampleSpecGetNumAttributes(es) ; j++) {
         is->deltaPlus[i][j] = fabs(is->deltaPlus[i][j]);
         is->deltaMinus[i][j] = fabs(is->deltaMinus[i][j]);
      }
   }

   MFreePtr(denomValues);
}

float IterationStatsErrorBoundDimension(IterationStatsPtr is, int cluster,
					int dimension) {
   return max(
    fabs(
     ExampleGetContinuousAttributeValue(VALIndex(is->cMax, cluster),
                                                             dimension) -
     ExampleGetContinuousAttributeValue(VALIndex(is->centroids, cluster),
                                                dimension)),
    fabs(
     ExampleGetContinuousAttributeValue(VALIndex(is->cMin, cluster), 
                                                               dimension) -
     ExampleGetContinuousAttributeValue(VALIndex(is->centroids, cluster),
                                                        dimension)));

}

IterationStatsPtr IterationStatsNext(IterationStatsPtr is, float delta, float R, float assignErrorScale, ExampleSpecPtr es, int usePedroBound, FILE *boundData) {
   IterationStatsPtr newIs;
   int i, j;
   float thisBound;
   VoidAListPtr newCentroids = VALNew();

   /* initialize the new stat structure with the correct centroids */
   _MakeNewCentroids(is->centroids, newCentroids, (VoidAListPtr)0, 
        is->wx, is->w, is->wPlusSquare, is->wMinus, is->n, es,
         gNeededDelta, 0, 0, 0, (double **) 0, (double **) 0);
   newIs = IterationStatsInitial(newCentroids);
   newIs->isFirstIteration = 0;

   /* HERE MEM this leaks the memory of the newCentroids should example free */
   VALFree(newCentroids);

   if(usePedroBound) {
      _RecordPedroBoundInfo(boundData, is, newIs->centroids, es);

      if(gMessageLevel > 1) {
	IterationStatsWrite(is, es, stdout);
      }

      /* now make the min and max centroids */
      _MakeNewCentroids(is->centroids, newIs->cMax, newIs->cMaxAssignment,
           is->wx, is->w, is->wPlusSquare, is->wMinus, is->n, es, gNeededDelta,
                1, 1, !is->isFirstIteration, is->deltaPlus, is->deltaMinus);
      _MakeNewCentroids(is->centroids, newIs->cMin, newIs->cMinAssignment, 
           is->wx, is->w, is->wPlusSquare, is->wMinus, is->n, es, gNeededDelta,
                1, 0, !is->isFirstIteration, is->deltaPlus, is->deltaMinus);
   } else {
      /* now make the min and max centroids */
      _MakeNewCentroids(is->centroids, newIs->cMax, newIs->cMaxAssignment,
              is->wxPlus, is->wMinus,
         is->wPlusSquare, is->wMinus, is->n, es, gNeededDelta, 1, 1, 0, 
                   (double **)0, (double **)0);
      _MakeNewCentroids(is->centroids, newIs->cMin, newIs->cMinAssignment, 
              is->wxMinus, is->wPlus,
         is->wPlusSquare, is->wMinus, is->n, es, gNeededDelta, 1, 0, 0,
                    (double **)0, (double **)0);
   }

   newIs->foundBound = is->foundBound;

   /* now fill in the bounds information */
   for(i = 0 ; i < VALLength(is->centroids) ; i++) {
      if(is->w[i] == 0 || is->wMinus[i] == 0 || is->wPlus[i] == 0) {
         newIs->lastBound[i] = 1000000;
         newIs->lastAssignmentBound[i] = 1000000;
         newIs->foundBound = 0;
         //printf("*** n^ == n+ can't find a bound for this run\n");
         //exit(0);
      } else {
         newIs->lastBound[i] = max(
             ExampleDistance(VALIndex(newIs->centroids, i),
                                      VALIndex(newIs->cMax, i)),
             ExampleDistance(VALIndex(newIs->centroids, i),
                                      VALIndex(newIs->cMin, i)));

         newIs->lastAssignmentBound[i] = max (
             ExampleDistance(VALIndex(newIs->centroids, i),
                                      VALIndex(newIs->cMaxAssignment, i)),
             ExampleDistance(VALIndex(newIs->centroids, i),
                                      VALIndex(newIs->cMinAssignment, i)));

         for(j = 0 ; j < ExampleSpecGetNumAttributes(es) ; j++) {
            /* Find the largest dimension bound so we know
                 how many samples for next round if needed */
            /* HERE discrete? */
            thisBound = IterationStatsErrorBoundDimension(newIs, i, j);

            if(newIs->maxEkd < thisBound) {
               newIs->maxEkd = thisBound;
            }
         }
      }
   }

   if(gMessageLevel > 2) {
      printf("Made the next IS, centroids:\n");
      for(i = 0 ; i < VALLength(newIs->centroids) ; i++) {
         printf("lastBound %d: %.4f lastAssignmentBound: %.4f\n", i, 
                 newIs->lastBound[i], newIs->lastAssignmentBound[i]);
         printf("nCMin   %d: ", i);
         ExampleWrite(VALIndex(newIs->cMin, i), stdout);
         printf("nCMinAs %d: ", i);
         ExampleWrite(VALIndex(newIs->cMinAssignment, i), stdout);
         printf("nC      %d: ", i);
         ExampleWrite(VALIndex(newIs->centroids, i), stdout);
         printf("nCMaxAs %d: ", i);
         ExampleWrite(VALIndex(newIs->cMaxAssignment, i), stdout);
         printf("nCMax   %d: ", i);
         ExampleWrite(VALIndex(newIs->cMax, i), stdout);
         fflush(stdout);
      }
   }


   return newIs;
}

void IterationStatsWrite(IterationStatsPtr is, ExampleSpecPtr es, FILE *out) {
   int i, j;

   fprintf(out, "----------\n");
   for(i = 0 ; i < VALLength(is->centroids) ; i++) {
      fprintf(out, "center #%d lastBound: %.3lf lastAssignmentBound: %.3f\n", 
            i, is->lastBound[i], is->lastAssignmentBound[i]);
      fprintf(out, "   ");
      ExampleWrite(VALIndex(is->cMin, i), out);
      fprintf(out, "   ");
      ExampleWrite(VALIndex(is->centroids, i), out);
      fprintf(out, "   ");
      ExampleWrite(VALIndex(is->cMax, i), out);
      fprintf(out, "   w- %.2lf w %.2lf w+ %.2lf w+^2 %.2lf\n", is->wMinus[i], is->w[i], is->wPlus[i], is->wPlusSquare[i]);
      for(j = 0 ; j < ExampleSpecGetNumAttributes(es) ; j++) {
         fprintf(out, "      d%d: wx- %.2lf wx %.2lf wx+ %.2lf delta+ %.2f delta- %.2f\n", j, is->wxMinus[i][j], is->wx[i][j], is->wxPlus[i][j], is->deltaPlus[i][j], is->deltaMinus[i][j]);
      }
   }
}
