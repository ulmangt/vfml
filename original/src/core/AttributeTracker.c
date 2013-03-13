#include "AttributeTracker.h"
#include "memory.h"

AttributeTrackerPtr AttributeTrackerNew(int numAttributes) {
   AttributeTrackerPtr at = MNewPtr(sizeof(AttributeTracker));
   int i;

   at->bits = BitFieldNew((numAttributes / 8) + 1);
   at->numAttributes = numAttributes;
   at->numActiveAttributes = numAttributes;

   for(i = 0 ; i < numAttributes ; i++) {
      BitFieldSetBit(at->bits, i, 1);
   }

   return at;
}

void AttributeTrackerFree(AttributeTrackerPtr at) {
   BitFieldFree(at->bits);
   MFreePtr(at);
}

AttributeTrackerPtr AttributeTrackerInitial(ExampleSpecPtr es) {
   int i;
   AttributeTrackerPtr at = 
            AttributeTrackerNew(ExampleSpecGetNumAttributes(es));

   for(i = 0 ; i < ExampleSpecGetNumAttributes(es) ; i++) {
      if(ExampleSpecGetAttributeType(es, i) == asIgnore) {
         AttributeTrackerMarkInactive(at, i);
      }
   }

   return at;
}

AttributeTrackerPtr AttributeTrackerClone(AttributeTrackerPtr at) {
   int i;
   AttributeTrackerPtr clone = AttributeTrackerNew(at->numAttributes);

   for(i = 0 ; i < at->numAttributes ; i++) {
      if(AttributeTrackerIsActive(at, i)) {
         AttributeTrackerMarkActive(clone, i);
      } else {
         AttributeTrackerMarkInactive(clone, i);
      }
   }

   return clone;
}

void AttributeTrackerMarkActive(AttributeTrackerPtr at, int attNum) {
  /* HERE may want to check for attNum in range */
   if(BitFieldGetBit(at->bits, attNum) == 0) {
      BitFieldSetBit(at->bits, attNum, 1);
      at->numActiveAttributes++;
   }
}

void AttributeTrackerMarkInactive(AttributeTrackerPtr at, int attNum) {
  /* HERE may want to check for attNum in range */
   if(BitFieldGetBit(at->bits, attNum) == 1) {
      BitFieldSetBit(at->bits, attNum, 0);
      at->numActiveAttributes--;
   }
}

int AttributeTrackerIsActive(AttributeTrackerPtr at, int attNum) {
  /* HERE may want to check for attNum in range */

   return BitFieldGetBit(at->bits, attNum);
}

int AttributeTrackerAreAllInactive(AttributeTrackerPtr at) {
   return at->numActiveAttributes == 0;
}

int AttributeTrackerNumActive(AttributeTrackerPtr at) {
   return at->numActiveAttributes;
}
