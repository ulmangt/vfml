#include "ExampleGenerator.h"
#include "random.h"
#include "memory.h"

ExampleGeneratorPtr ExampleGeneratorNew(ExampleSpecPtr es, int seed) {
   ExampleGeneratorPtr eg = MNewPtr(sizeof(ExampleGenerator));

   if(seed == -1) {
      seed = RandomRange(0, RAND_MAX - 1);
   }

   /* sets a new state then restores the origional and saves the new */
   eg->randomState = RandomNewState(seed);
   eg->randomState = RandomSetState(eg->randomState);

   eg->spec = es;
   eg->numGenerated = 0;
   eg->maxGenerated = -1;

   return eg;
}

void ExampleGeneratorFree(ExampleGeneratorPtr eg) {
   RandomFreeState(eg->randomState);
   MFreePtr(eg);
}

static char *_EGMakeString(int i) {
  /* a bit twisted so that we can support huge numbers without having to
     waste memory on every one of them, Damn, but ints only go so high,
     oh well*/
   char tmp[500];
   char *ret;

   sprintf(tmp, "%d", i);

   ret = MNewPtr(sizeof(char) * strlen(tmp) + 1);

   sprintf(ret, "%d", i);
   return ret;
}

ExamplePtr ExampleGeneratorGenerate(ExampleGeneratorPtr eg) {
   int i, j;
   ExamplePtr e;
   void *savedState;

   if(eg->maxGenerated != -1) {
      if(eg->numGenerated >= eg->maxGenerated) {
         return 0;
      }
   }

   savedState = RandomSetState(eg->randomState);
   eg->numGenerated++;

   e = ExampleNew(eg->spec);

   for(i = 0 ; i < ExampleSpecGetNumAttributes(eg->spec) ; i++) {
      switch(ExampleSpecGetAttributeType(eg->spec, i)) {
       case asIgnore:
         ExampleSetAttributeUnknown(e, i);
         break;
       case asContinuous:
         ExampleSetContinuousAttributeValue(e, i, RandomDouble());
         break;
       case asDiscreteNamed:
         ExampleSetDiscreteAttributeValue(e, i, RandomRange(0, 
                ExampleSpecGetAttributeValueCount(eg->spec, i) - 1));
         break;
       case asDiscreteNoName:
         for(j = ExampleSpecGetAssignedAttributesValueCount(eg->spec, i)
                ; j < ExampleSpecGetAttributeValueCount(eg->spec, i) ; j++) {
            ExampleSpecAddAttributeValue(eg->spec, i, _EGMakeString(j));
         }

         ExampleSetDiscreteAttributeValue(e, i, RandomRange(0, 
                ExampleSpecGetAttributeValueCount(eg->spec, i) - 1));
         break;
      }
   }
   ExampleSetClassUnknown(e);

   RandomSetState(savedState);
   return e;
}
