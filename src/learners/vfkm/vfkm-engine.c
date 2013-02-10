#include "vfkm-engine.h"
#include <math.h>

extern int gUseNormalApprox;

IterationStatsPtr IterationStatsInitial(VoidAListPtr initialClusters) {
   IterationStatsPtr is;
   int i;

   is = MNewPtr(sizeof(IterationStats));
   is->centroids = VLNew();
   for(i = 0 ; i < VALLength(initialClusters) ; i++) {
      VALAppend(is->centroids, ExampleClone(VALIndex(initialClusters, i)));
   }

   is->nHat = MNewPtr(sizeof(long) * VALLength(is->centroids));
   is->nPlus = MNewPtr(sizeof(long) * VALLength(is->centroids));
   is->nMinus = MNewPtr(sizeof(long) * VALLength(is->centroids));
   is->lastBound = MNewPtr(sizeof(float) * VALLength(is->centroids));
   is->errorBound = MNewPtr(sizeof(float *) * VALLength(is->centroids));
   is->deltaPlus = MNewPtr(sizeof(float *) * VALLength(is->centroids));
   is->deltaMinus = MNewPtr(sizeof(float *) * VALLength(is->centroids));
   is->xMaxSquareSum = MNewPtr(sizeof(double *) * VALLength(is->centroids));
   is->xMinSum = MNewPtr(sizeof(double *) * VALLength(is->centroids));
   is->wonSum = MNewPtr(sizeof(double *) * VALLength(is->centroids));
   for(i = 0 ; i < VALLength(is->centroids) ; i++) {
      is->errorBound[i] = MNewPtr(sizeof(float) * 
                 ExampleGetNumAttributes(VALIndex(is->centroids, 0)));
      is->deltaPlus[i] = MNewPtr(sizeof(float) * 
                 ExampleGetNumAttributes(VALIndex(is->centroids, 0)));
      is->deltaMinus[i] = MNewPtr(sizeof(float) * 
                 ExampleGetNumAttributes(VALIndex(is->centroids, 0)));
      is->xMaxSquareSum[i] = MNewPtr(sizeof(double) * 
                 ExampleGetNumAttributes(VALIndex(is->centroids, 0)));
      is->xMinSum[i] = MNewPtr(sizeof(double) * 
                 ExampleGetNumAttributes(VALIndex(is->centroids, 0)));
      is->wonSum[i] = MNewPtr(sizeof(double) * 
                 ExampleGetNumAttributes(VALIndex(is->centroids, 0)));
   }

   IterationStatsClearCounts(is);

   /* we always have a perfect bound for an initial is */
   is->foundBound = 1;

   return is;
}

void IterationStatsClearCounts(IterationStatsPtr is) {
   int i, j;

   is->n = 0;

   for(i = 0 ; i < VALLength(is->centroids) ; i++) {
      is->nHat[i] = 0;
      is->nPlus[i] = 0;
      is->nMinus[i] = 0;

      is->lastBound[i] = 0;
      for(j = 0 ; j < ExampleGetNumAttributes(VALIndex(is->centroids, 0)) ; j++) {
         is->errorBound[i][j] = 0;
         is->deltaPlus[i][j] = 0;
         is->deltaMinus[i][j] = 0;
         is->xMaxSquareSum[i][j] = 0;
         is->xMinSum[i][j] = 0;
         is->wonSum[i][j] = 0;
      }
   }

   is->maxEkd = 0;
   is->possibleIDConverge = 0;
   is->guarenteeIDConverge = 0;
   is->wouldKMConverge = 0;
   is->convergeVFKM = 0;
}

void IterationStatsFree(IterationStatsPtr is) {
   int numCentroids = VALLength(is->centroids);
   int i;
   ExamplePtr e;

   for(i = 0 ; i < numCentroids ; i++) {
      e = VALIndex(is->centroids, i);
      ExampleFree(e);
   }
   VALFree(is->centroids);

   MFreePtr(is->lastBound);
   MFreePtr(is->nHat);
   MFreePtr(is->nPlus);
   MFreePtr(is->nMinus);
   for(i = 0 ; i < numCentroids ; i++) {
      MFreePtr(is->errorBound[i]);
      MFreePtr(is->deltaPlus[i]);
      MFreePtr(is->deltaMinus[i]);
      MFreePtr(is->xMaxSquareSum[i]);
      MFreePtr(is->xMinSum[i]);
      MFreePtr(is->wonSum[i]);
   }
   MFreePtr(is->errorBound);
   MFreePtr(is->deltaPlus);
   MFreePtr(is->deltaMinus);
   MFreePtr(is->xMaxSquareSum);
   MFreePtr(is->xMinSum);
   MFreePtr(is->wonSum);

   MFreePtr(is);
}


static void _MakeNewCentroids(VoidAListPtr centers, VoidAListPtr newCenters,
          double **sumsList, long *num, ExampleSpecPtr es) {
   double *sums;
   ExamplePtr newCenter, e;
   int i,j;

   /* free any old centers from the list */
   while(VALLength(newCenters) > 0) {
      i = VALLength(newCenters) - 1;
      e = VALIndex(newCenters, i);
      ExampleFree(e);
      VALRemove(newCenters, i);
   }

   /* make the list of new centers */
   for(i = 0 ; i < VALLength(centers) ; i++) {
      newCenter = ExampleNew(es);
      sums = sumsList[i];

      if(gMessageLevel > 1 && num[i] == 0) {
         printf("No examples assigned to center: ");
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
      //printf("distance #%d %f\n", i, ExampleDistance(newCenter, VALIndex(centers,i)));
      //printf("c_old%d: ", i);
      //ExampleWrite(VALIndex(centers, i), stdout);
      //printf("c_new%d: ", i);
      //ExampleWrite(newCenter, stdout);


      VALAppend(newCenters, newCenter);
   }
}


IterationStatsPtr IterationStatsNext(IterationStatsPtr is, float delta, float R, float assignErrorScale, ExampleSpecPtr es) {
   IterationStatsPtr newIs;
   int i, j;
   float hoeffdingBound, variance, normalBound=0, usedBound;
   double thisBound, boundSquareSum;
   VoidAListPtr newCentroids = VALNew();

   /* initialize the new stat structure with the correct centroids */
   _MakeNewCentroids(is->centroids, newCentroids, is->wonSum, is->nHat, es);
   newIs = IterationStatsInitial(newCentroids);
   VALFree(newCentroids);

   newIs->foundBound = is->foundBound;

   /* now fill in the bounds information */
   for(i = 0 ; i < VALLength(is->centroids) ; i++) {
      boundSquareSum = 0;

      if(is->nHat[i] - is->nPlus[i] == 0) {
         newIs->lastBound[i] = 1000000;
         newIs->foundBound = 0;
         //printf("*** n^ == n+ can't find a bound for this run\n");
         //exit(0);
      } else {

         hoeffdingBound = sqrt((double)(R * R * log(2 / delta)) / 
                       (double)(2.0 * (is->nHat[i] - is->nPlus[i])));

         for(j = 0 ; j < ExampleSpecGetNumAttributes(es) ; j++) {
            if(gUseNormalApprox) {
               variance = (is->nHat[i] + is->nMinus[i]);
               variance *= is->xMaxSquareSum[i][j];
               variance -= pow(is->xMinSum[i][j], 2);
               variance /= (float)(is->nHat[i] - is->nPlus[i]);
               variance /= (float)(is->nHat[i] - is->nPlus[i] - 1);

               normalBound = StatGetNormalBound(variance, (is->nHat[i] - is->nPlus[i]), delta);

               DebugMessage(1, 0, "c%dd%d n^ %ld delta %f hoeffding %f variance %f normal %f n-advant %f\n", i, j, is->nHat[i], delta, hoeffdingBound, variance, normalBound, hoeffdingBound / normalBound);

            }

            if(gUseNormalApprox && (normalBound < hoeffdingBound)) {
               usedBound = normalBound;
            } else {
               usedBound = hoeffdingBound;
            }

            /* the denominator is > 0 because of a check just above */
            thisBound = (1.0/(float)(is->nHat[i] - is->nPlus[i]));
            thisBound *= max(is->deltaPlus[i][j], is->deltaMinus[i][j]);
            thisBound *= assignErrorScale;
            thisBound += usedBound;
            if(newIs->maxEkd < thisBound) {
               newIs->maxEkd = thisBound;
            }

            newIs->errorBound[i][j] = thisBound;
            boundSquareSum += thisBound * thisBound;
         }

         newIs->lastBound[i] = sqrt(boundSquareSum);
      }
   }

   return newIs;
}

void IterationStatsWrite(IterationStatsPtr is, FILE *out) {
   int i;

   fprintf(out, "----------\n");
   for(i = 0 ; i < VALLength(is->centroids) ; i++) {
      fprintf(out, "c #%d lb: %.3lf n^: %ld n+: %ld n-: %ld\n", i, is->lastBound[i], is->nHat[i], is->nPlus[i], is->nMinus[i]);
   }
}

