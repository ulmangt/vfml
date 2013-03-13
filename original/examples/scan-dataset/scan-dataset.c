#include "uwml.h"
#include <stdio.h>

/* used to keep statistics about the examples */
int gNumExamples = 0;
int gNumSpoiled = 0;
double gSumDays = 0.0;
int gNumDaysUnknown = 0;
int gNumFewSpots = 0;

int main(void) {
   ExampleSpecPtr es = ExampleSpecRead("test.names");
   ExamplePtr e;
   FILE *exampleIn = fopen("test.data", "r");

   /* lets examine the ExampleSpec a bit */
   printf("There are %d attributes.\n", ExampleSpecGetNumAttributes(es));
   printf("There are %d classes.\n", ExampleSpecGetNumClasses(es));

   if(ExampleSpecIsAttributeContinuous(es, 0)) {
      printf("  Attribute with index 0 is continuous.\n");
   }

   if(ExampleSpecIsAttributeDiscrete(es, 1)) {
      printf("  Attribute with index 1 is discrete and has %d values.\n",
            ExampleSpecGetAttributeValueCount(es, 1));
   }

   /* and now to examine the data */
   printf("\nscanning data...\n\n");

   e = ExampleRead(exampleIn, es);
   while(e != 0) { /* ExampleRead returns 0 when EOF */
      /* keep a count of the examples */
      gNumExamples++;

      /* keep a count of how many of them are spoiled */
      if(!ExampleIsClassUnknown(e)) {
         if(ExampleGetClass(e) == 1) {
            gNumSpoiled++;
         }
      }

      /* keep a sum of the number of days */
      if(!ExampleIsAttributeUnknown(e, 0)) {
         gSumDays += ExampleGetContinuousAttributeValue(e, 0);
      } else {
         gNumDaysUnknown++;
      }

      /* keep a total of the number of bananas that have a few spots */
      if(!ExampleIsAttributeUnknown(e, 1)) {
         if(ExampleGetDiscreteAttributeValue(e, 1) == 1) {
            gNumFewSpots++;
         }
      }

      /* now move on to the next example */
      ExampleFree(e);
      e = ExampleRead(exampleIn, es);
   }


   /* print out the statistics recorded during the scan */
   printf("There were %d bananas.\n", gNumExamples);

   printf("%.2f percent were spoiled.\n",
                ((float)gNumSpoiled / (float)gNumExamples) * 100);
   printf("Average number of days on the table %.2lf.\n", 
            gSumDays / (gNumExamples - gNumDaysUnknown));
   printf("%d of them had a few spots.\n", gNumFewSpots);
}


