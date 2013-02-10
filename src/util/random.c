#include "sysdefines.h"
#include "Debug.h"
#include "random.h"
#include "memory.h"
#include <time.h>
#include <math.h>

#if defined(WIN32)
#define srandom srand
#define random rand 
#define M_PI 3.14159265358979323846264338327
#endif

void RandomInit(void) {
   srandom(time(NULL));
}

static long _SysGetRandom(void) {
   return random();
}

static void _SysRandomSeed(unsigned int seed) {
   srandom(seed);
}

int RandomRange(int min, int max) {
   int tmpNum;
   if (min < max) {
      tmpNum = _SysGetRandom();
      tmpNum = min + (int)(((float)((max - min) + 1) * tmpNum) / (RAND_MAX + 1.0));
      return tmpNum;
   } else if (min == max) {
      return (min);
   } else {
      DebugWarn(1, "Possible Error in RandomRange: min > max.");
      return min;
   }
}

void RandomSeed(unsigned int seed) {
   _SysRandomSeed(seed);
}

long RandomLong(void) {
   return _SysGetRandom();
}

double RandomDouble(void) {
   return ((double)RandomRange(0, 30000)) / 30000.0;
}

double RandomStandardNormal(void) {
   /* Generates a number according to a standard normal distribution using
         the Box-Mulder method */
   double u1, u2;
   u1 = u2 = 0;

   while(u1 * u2 == 0) {
      u1 = RandomDouble();
      u2 = RandomDouble();
   }
   return sqrt(-2. * log(u1)) * cos(2 * M_PI * u2);
}

double RandomGaussian(double mean, double stdev) {
   return stdev * RandomStandardNormal() + 1. * mean;
}


#if defined(WIN32)
 /* WIN32 doesn't support this state stuff, HERE I better do something! */
 void *RandomSetState(void *state) {
    return 0;
 }

 void *RandomNewState(unsigned int seed) {
    return 0;
 }

 void RandomFreeState(void *state) {
 }

#else

 void *RandomSetState(void *state) {
    return (void *)setstate(state);
 }

 void *RandomNewState(unsigned int seed) {
   return (void *)initstate(seed, MNewPtr(300), 16);
 }

 void RandomFreeState(void *state) {
   MFreePtr(state);
 }
#endif
