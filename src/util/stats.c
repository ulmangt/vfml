#include "stats.h"
#include "memory.h"
#include <math.h>

StatTracker StatTrackerNew(void) {
   StatTracker st = MNewPtr(sizeof(StatTrackerStruct));

   st->n = 0;
   st->sum = 0;
   st->sumSquare = 0;

   return st;
}

void StatTrackerFree(StatTracker st) {
   MFreePtr(st);
}

void StatTrackerAddSample(StatTracker st, double x) {
   st->n++;
   st->sum += x;
   st->sumSquare += (x * x);
}

double StatTrackerGetMean(StatTracker st) {
   return st->sum / (double)st->n;
}

long StatTrackerGetNumSamples(StatTracker st) {
   return st->n;
}

double StatTrackerGetVariance(StatTracker st) {
   double n = (double)st->n;

   return ((n * st->sumSquare) - (st->sum * st->sum)) / (n * (n - 1));
}

double StatTrackerGetStdev(StatTracker st) {
   return sqrt(StatTrackerGetVariance(st));
}

const static double _ZDelta[11] = {.1, .01, .001, .0001, .00001, .000001, 0.000000287104999663335, 0.0000000009901218733787690, 0.0000000000012880807531701, 0.0000000000000321964677141, -1};
const static double _ZValue[11] = {1.281550794, 2.326341928, 3.090244718, 3.719469532, 4.265457392, 4.768371582, 5, 6, 7, 7.5, 8};

static double _GetZValue(double delta) {
   int i, found;
   double pre, post, scale;

   /* do a linear search through the parallel arrays _ZDelta and _ZValue,
       return a linear interpolation between the values assoced with  
       the two closest deltas.  If requested delta bigger or smaller
       just return the max _ZValue in that direction */
   found = 0;
   for(i = 0 ; _ZDelta[i] != -1 && !found; i++) {
      if(_ZDelta[i] <= delta) {
         if(i == 0) {
            /* requested delta is bigger than any in the table */
            //printf("before i: %d\n");
            return _ZValue[0];
         } else {
            pre = _ZValue[i - 1];
            post = _ZValue[i];
            scale = 1.0 - (delta - _ZDelta[i]) / (_ZDelta[i - 1] - _ZDelta[i]);

            //printf("i: %d pre: %f post: %f scale: %f\n", i, pre, post, scale);
            return ((post - pre) * scale) + pre;
         }
      }
   }

   /* requested delta is smaller than any in the table */
   //printf("i: %d val %lf\n", i, );

   return _ZValue[i];
}

double StatTrackerGetNormalBound(StatTracker st, double delta) {
   double stdev = StatTrackerGetStdev(st);
   double rootN = sqrt((double)st->n);

   return _GetZValue(delta) * (stdev / rootN);
}

double StatGetNormalBound(double variance, long n, double delta) {
   double stdev = sqrt(variance);
   double rootN = sqrt(n);

   return _GetZValue(delta) * (stdev / rootN);
}

double StatHoeffdingBoundOne(double range, double delta, long n) {
   return sqrt(((range * range) * log(1.0/delta)) / (double)(2.0 * n));
}

double StatHoeffdingBoundTwo(double range, double delta, long n) {
   return sqrt(((range * range) * log(2.0/delta)) / (double)(2.0 * n));
}

float StatLogGamma(float xx) {
   /* (C) Copr. 1986-92 Numerical Recipes Software 0H21$1-V'13. */

   double x,y,tmp,ser;
   static double cof[6]={76.18009172947146,-86.50532032941677,
                24.01409824083091,-1.231739572450155,
                0.1208650973866179e-2,-0.5395239384953e-5};
   int j;

   y=x=xx;
   tmp=x+5.5;
   tmp -= (x+0.5)*log(tmp);
   ser=1.000000000190015;
   for (j=0;j<=5;j++) ser += cof[j]/++y;
   return -tmp+log(2.5066282746310005*ser/x);
}
