#ifndef VFKMH
#define VFKMH

#include "vfml.h"

extern int gMessageLevel;

typedef struct _ITERATIONSTATS_ {
   VoidAListPtr centroids;
   float *lastBound;
   float maxEkd;
   int   foundBound;
   long *nHat;
   long *nPlus;
   long *nMinus;
   long n;
   int possibleIDConverge;
   int guarenteeIDConverge;
   int wouldKMConverge;
   int convergeVFKM;

   /* a float for each cluster, for each dimension */
   float **errorBound;
   float **deltaPlus;
   float **deltaMinus;
   double **xMaxSquareSum;
   double **xMinSum;
   double **wonSum;
} IterationStats, *IterationStatsPtr;

IterationStatsPtr IterationStatsInitial(VoidAListPtr initialClusters);
void IterationStatsFree(IterationStatsPtr is);
void IterationStatsClearCounts(IterationStatsPtr is);
IterationStatsPtr IterationStatsNext(IterationStatsPtr is,
                            float delta, float R, float assignErrorScale,
                                   ExampleSpecPtr es);
void IterationStatsWrite(IterationStatsPtr is, FILE *out);

#endif
