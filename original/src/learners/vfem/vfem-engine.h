#ifndef VFEMH
#define VFEMH

#include "vfml.h"

extern int gMessageLevel;

typedef struct _ITERATIONSTATS_ {
   VoidAListPtr centroids;
   VoidAListPtr cMax;
   VoidAListPtr cMaxAssignment;
   VoidAListPtr cMin;
   VoidAListPtr cMinAssignment;
   float *lastBound;
   float *lastAssignmentBound;
   float maxEkd;
   int   foundBound;
   int isFirstIteration;
   long n;
   int possibleIDConverge;
   int guarenteeIDConverge;
   int wouldEMConverge;
   int convergeVFEM;

   /* a float for each cluster */
   double *w;
   double *wPlus;
   double *wPlusSquare;
   double *wMinus;

   /* for each cluster and each dimension */
   double **wx;
   double **wxPlus;
   double **wxMinus;

   double **deltaPlus;
   double **deltaMinus;
} IterationStats, *IterationStatsPtr;

IterationStatsPtr IterationStatsInitial(VoidAListPtr initialClusters);
void IterationStatsFree(IterationStatsPtr is);
void IterationStatsClearCounts(IterationStatsPtr is);
float IterationStatsErrorBoundDimension(IterationStatsPtr is, int cluster,
                                 int dimension);
IterationStatsPtr IterationStatsNext(IterationStatsPtr is,
                            float delta, float R, float assignErrorScale,
                                   ExampleSpecPtr es, int usePedroBound,
                                    FILE *boundData);

void IterationStatsWrite(IterationStatsPtr is, ExampleSpecPtr es, FILE *out);


#endif
