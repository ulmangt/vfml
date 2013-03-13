#include "vfml.h"
#include <string.h>
#include <stdio.h>

int gAddValue = 0;
int gRemoveIgnore = 0;

#define MOSTCOMMON 0
#define DELETE 1
#define ADD 2
#define AUTO 3
#define NOTHING 4
  
static char *_AllocateString(char *first) {
   char *second = MNewPtr(sizeof(char) * (strlen(first) + 1));

   strcpy(second, first);

   return second;
}

static ExampleSpecPtr _MakeNewExampleSpecForIgnores(ExampleSpecPtr es) {
   int i, j, k;

   ExampleSpecPtr newSpec = ExampleSpecNew();

   for(i = 0 ; i < ExampleSpecGetNumClasses(es) ; i++) {
      ExampleSpecAddClass(newSpec, 
            _AllocateString(ExampleSpecGetClassValueName(es, i)));
   }

   k = 0;
   for(i = 0 ; i < ExampleSpecGetNumAttributes(es) ; i++) {
      if(!(ExampleSpecIsAttributeIgnored(es, i) && gRemoveIgnore)) {
         if(ExampleSpecIsAttributeDiscrete(es, i)) {
            ExampleSpecAddDiscreteAttribute(newSpec, 
               _AllocateString(ExampleSpecGetAttributeName(es, i)));

            for(j = 0 ; j < ExampleSpecGetAttributeValueCount(es, i) ; j++) {
               ExampleSpecAddAttributeValue(newSpec, k,
                  _AllocateString(ExampleSpecGetAttributeValueName(es, i, j)));
            }
         } else if(ExampleSpecIsAttributeContinuous(es, i)) {
            ExampleSpecAddContinuousAttribute(newSpec, 
               _AllocateString(ExampleSpecGetAttributeName(es, i)));
         } else if(ExampleSpecIsAttributeIgnored(es, i)) {
            ExampleSpecAddContinuousAttribute(newSpec, 
               _AllocateString(ExampleSpecGetAttributeName(es, i)));
            ExampleSpecIgnoreAttribute(newSpec, i);
         } else {
            DebugError(1, "Unsupported attribute type\n");
         }
         k++;
      }
   }
   return newSpec;
}

int main(int argc, char *argv[]) {
   char fileNames[255] = "/0";
   char outfileNames[255] = "/0";
   char *gFileStem = "DF";
   char *gSourceDirectory = ".";

   FILE *exampleIn, *exampleOut;
   ExamplePtr e, newE;
   ExampleSpecPtr gEs;
   ExampleSpecPtr gnewEs;
   float *classCounts;
   int i, j, k;
   int gHasContinuous = 0;
   int mostCommonClass;
   float count = 0;   /* for counting the number of example */
   float *** prob; /* for store probability for each value of attributes */

   float ** mostCommonValue;
   float temp;     /* for temperary storing some double values */
   float *unknownCount; /* for counting the number of unKnown */
   int *fillMode; /* for determining the way a unknown attribute is filled */

   
   /* This processes the command line arguments and sets some globals */
   for(i = 1; i < argc; i++)
   {
     if(!strcmp(argv[i], "-f")){
       gFileStem = argv[i+1];
       /* ignore the next argument */
       i++;
     } else if(!strcmp(argv[i], "-source")){
       gSourceDirectory = argv[i+1];
       i++;
     } else if(!strcmp(argv[i], "-addValue")){
       gAddValue = 1;
     } else if(!strcmp(argv[i], "-removeIgnore")){
       gRemoveIgnore = 1;
     } else if(!strcmp(argv[i], "-v")){
       DebugSetMessageLevel(DebugGetMessageLevel() + 1);
     } else if(!strcmp(argv[i], "-h")){
       printf("-f <filestem>\tSet the name of the dataset (default DF)\n");
       printf("-source <dir>\tSet the source data directory (default '.')\n");
       printf("-addValue adds an 'unknown' value to all categorical (default fill\n\t\t\tin most common)\n");
       printf("-removeIgnore removes every attribute marked 'ignored' in names from \n\t\t\tthe data set (default leave them in)\n");
       printf("-v \t\tUse multiple times to increase debugging output\n");
       exit(0);
     } else {
       printf("Unknown argument: %s, use -h for help\n", argv[i]);
       exit(0);
     }
   }

   /* open the names file and read the example spec into a global */
   sprintf(fileNames, "%s/%s.names", gSourceDirectory, gFileStem);
   gEs = ExampleSpecRead(fileNames);
   DebugError(gEs == 0, "Unable to open the .names file");

   /* initialize the class counts */
   classCounts = MNewPtr(sizeof(float) * ExampleSpecGetNumClasses(gEs));
   prob = MNewPtr (sizeof(float**) *ExampleSpecGetNumClasses(gEs));
   mostCommonValue = MNewPtr (sizeof(float*) *ExampleSpecGetNumClasses(gEs));
   unknownCount = MNewPtr(sizeof(float)*ExampleSpecGetNumAttributes(gEs));
   fillMode = MNewPtr(sizeof(int)*ExampleSpecGetNumAttributes(gEs));
 
   /* build 3D array for holding the probability of each value 
      in each attribute according to the classValue */

   for(i = 0; i < ExampleSpecGetNumClasses(gEs); i++) {
      /* HERE HERE This code is a bit fishy to me */
      prob[i] = MNewPtr (sizeof(float*) *ExampleSpecGetNumAttributes(gEs));
      mostCommonValue[i] = MNewPtr (sizeof(float) 
				   *ExampleSpecGetNumAttributes(gEs));

      for(j = 0; j < ExampleSpecGetNumAttributes(gEs); j++) {
         mostCommonValue[i][j] = 0;
	  
         if(ExampleSpecIsAttributeDiscrete(gEs, j)) {
            /* Allocate double spaces for each value in the attribute */
            prob[i][j] = 
              MNewPtr(sizeof(float)*ExampleSpecGetAttributeValueCount(gEs, j));

            for(k = 0; k < ExampleSpecGetAttributeValueCount(gEs, j); k++) {
               prob[i][j][k] = 0;
            }
         } else {	  /* Continuous */
            gHasContinuous = 1;
            /* Allocate a new double for storing the mean value */
            prob[i][j] = MNewPtr(sizeof(float)*1);
            prob[i][j][0] = 0;
         }
      }
   }
   
   

   /* initialize the classCounts */
   for(i = 0 ; i < ExampleSpecGetNumClasses(gEs) ; i++) {
      classCounts[i] = 0;
   }

   for(i = 0; i < ExampleSpecGetNumAttributes(gEs); i++) {
      unknownCount[i] = 0;
      fillMode[i] = MOSTCOMMON;
   }

 


   if(gHasContinuous || !gAddValue) {
      /* open the data file and do a scan to count */

      DebugMessage(1, 1, "Scanning data set once to gather counts...");
      sprintf(fileNames, "%s/%s.data", gSourceDirectory, gFileStem);
      exampleIn = fopen(fileNames, "r");
      DebugError(exampleIn == 0, "Unable to open the data file");
  
      /* scan the data and count the classes */
      count = 0;
      e = ExampleRead(exampleIn, gEs);
      while(e != 0) {
         count++;
     
         /* count the known class in the array */
         if(!ExampleIsClassUnknown(e))  {
            classCounts[ExampleGetClass(e)]++;
       
            for(i = 0; i < ExampleSpecGetNumAttributes(gEs); i++) {
               if(ExampleIsAttributeUnknown(e, i)) {
                  unknownCount[i]++;
               } else if(ExampleSpecIsAttributeDiscrete(gEs, i)) {
                  prob[ExampleGetClass(e)][i]
                      [ExampleGetDiscreteAttributeValue(e, i)]++;
               } else {
                  prob[ExampleGetClass(e)][i][0] += 
                     ExampleGetContinuousAttributeValue(e, i);
               }
            }
         }
     
         ExampleFree(e);
         e = ExampleRead(exampleIn, gEs); 
      }
      DebugMessage(1, 1, "  done...\n");
   }
	  
   /* find the mostCommonClass */
   mostCommonClass = 0;
   for(i = 0 ; i < ExampleSpecGetNumClasses(gEs) ; i++) {
      if(classCounts[i] > classCounts[mostCommonClass]) {
         mostCommonClass = i;
      }
   }

 
   /* Compute the most common value for each discrete attributevalue
    and mean for each continuous attribute */
   
   for(i = 0; i < ExampleSpecGetNumClasses(gEs); i++) {
      for(j = 0; j < ExampleSpecGetNumAttributes(gEs); j++) {
         if(ExampleSpecIsAttributeDiscrete(gEs, j)) {
            for(k = 0; k < ExampleSpecGetAttributeValueCount(gEs, j) ; k++) {
               if(prob[i][j][k] > mostCommonValue[i][j]) {
                  mostCommonValue[i][j] = k;
               }
            }
         } else {	 /* Continuous */
            if(classCounts[i] > 0) {
               prob[i][j][0] /= classCounts[i];
            } else {
               DebugMessage(1, 2, "No observations for attrib %d class %d\n",
                   j, i);
            }
         }
      }
   }

   for(i = 0; i < ExampleSpecGetNumAttributes(gEs); i++) {
      if(gAddValue && ExampleSpecIsAttributeDiscrete(gEs, i)) {
         fillMode[i] = ADD;
         ExampleSpecAddAttributeValue(gEs, i, "u");
      } else {
         fillMode[i] = MOSTCOMMON;
      }
   }    

   
   /* Write the name file */
   sprintf(outfileNames, "%s/%s-clean.names", gSourceDirectory,
	   gFileStem);
   
   exampleOut = fopen(outfileNames, "w");
   if(gRemoveIgnore) {
      gnewEs = _MakeNewExampleSpecForIgnores(gEs);
   } else {
      gnewEs = gEs;
   }

   ExampleSpecWrite(gnewEs, exampleOut);
   fclose(exampleOut);

	   
   /* reopen the file */
   DebugMessage(1, 1, "Doing a pass over train data and outputting cleaned file...\n");
   sprintf(fileNames, "%s/%s.data", gSourceDirectory, gFileStem);
   exampleIn = fopen(fileNames, "r");
   
   sprintf(outfileNames, "%s/%s-clean.data", gSourceDirectory,
	   gFileStem);
   exampleOut = fopen(outfileNames, "w");
   e = ExampleRead(exampleIn, gEs);
   newE = ExampleNew(gnewEs);
   while(e != 0) {
      /* if the example has unknown class, assigned the
	most common class to it */
      if(ExampleIsClassUnknown(e)) {
	  ExampleSetClass(newE, mostCommonClass);
      } else {
          ExampleSetClass(newE, ExampleGetClass(e));
      }

      k = 0;
      for(j = 0; j < ExampleSpecGetNumAttributes(gEs); j++) {
         if((gRemoveIgnore && ExampleSpecIsAttributeIgnored(gEs, j))) {
            /* skip it */
            //k--;
         } else {
            if(ExampleIsAttributeUnknown(e, j)) {
               if(ExampleIsAttributeDiscrete(e, j)) {
                  if(fillMode[j] == MOSTCOMMON) {
                     temp = mostCommonValue[ExampleGetClass(e)][j];
                     ExampleSetDiscreteAttributeValue(newE, k, temp);
                  } else if(fillMode[j] == ADD) {
                     ExampleSetDiscreteAttributeValue(newE, k, 
                          ExampleSpecGetAttributeValueCount(gEs, j)-1);
                  } else {
                     /* fill mode must be 'nothing' */
                     ExampleSetAttributeUnknown(newE, k);
                  }
               } else { 
                  if(fillMode[j] != NOTHING) {
                    ExampleSetContinuousAttributeValue(newE, k, 
                           prob[ExampleGetClass(e)][j][0]);
                  } else {
                     ExampleSetAttributeUnknown(newE, k);
                  }
               }
            }  else {
               if(ExampleIsAttributeDiscrete(e, j)) {
                  ExampleSetDiscreteAttributeValue(newE, k, 
                     ExampleGetDiscreteAttributeValue(e, j));
               } else {
                  /* must be continuous */
                  ExampleSetContinuousAttributeValue(newE, k, 
                    ExampleGetContinuousAttributeValue(e, j));

               }
            }
            k++;
         }
      }
   	  
      /* write the Example on another file */
      ExampleWrite(newE, exampleOut);
	  

      ExampleFree(e);
      e = ExampleRead(exampleIn, gEs);
   }
   ExampleFree(newE);
	  
   fclose(exampleIn);
   printf("size %f\n", count);	
   fclose(exampleOut);



   /* now do the test data */
   DebugMessage(1, 1, "Doing a pass over test data and outputting cleaned file...\n");
   sprintf(fileNames, "%s/%s.test", gSourceDirectory, gFileStem);
   sprintf(outfileNames, "%s/%s-clean.test", gSourceDirectory,
	   gFileStem);

   exampleIn = fopen(fileNames, "r");
   if(exampleIn != NULL) {
      /* convert the test file, otherwise there probably is no test file */
      exampleOut = fopen(outfileNames, "w");
  
      e = ExampleRead(exampleIn, gEs);
      newE = ExampleNew(gnewEs);
      while(e != 0) {
  
         /* if the example has unknown class, assigned the
	   most common class to it */
         if(ExampleIsClassUnknown(e)) {
	     ExampleSetClass(newE, mostCommonClass);
         } else {
             ExampleSetClass(newE, ExampleGetClass(e));
         }

         k = 0;
         for(j = 0; j < ExampleSpecGetNumAttributes(gEs); j++) {
            if((gRemoveIgnore && ExampleSpecIsAttributeIgnored(gEs, j))) {
               /* skip it */
               //k--;
            } else {
               if(ExampleIsAttributeUnknown(e, j)) {
                  if(ExampleIsAttributeDiscrete(e, j)) {
                     if(fillMode[j] == MOSTCOMMON) {
                        temp = mostCommonValue[ExampleGetClass(e)][j];
                        ExampleSetDiscreteAttributeValue(newE, k, temp);
                     } else if(fillMode[j] == ADD) {
                        ExampleSetDiscreteAttributeValue(newE, k, 
                             ExampleSpecGetAttributeValueCount(gEs, j)-1);
                     } else {
                        /* fill mode must be 'nothing' */
                        ExampleSetAttributeUnknown(newE, k);
                     }
                  } else { 
                     if(fillMode[j] != NOTHING) {
                        ExampleSetContinuousAttributeValue(newE, k, 
                              prob[ExampleGetClass(e)][j][0]);
                     } else {
                        ExampleSetAttributeUnknown(newE, k);
                     }
                  }
               }  else {
                  if(ExampleIsAttributeDiscrete(e, j)) {
                     ExampleSetDiscreteAttributeValue(newE, k, 
                        ExampleGetDiscreteAttributeValue(e, j));
                  } else {
                     /* must be continuous */
                     ExampleSetContinuousAttributeValue(newE, k, 
                       ExampleGetContinuousAttributeValue(e, j));

                  }
               }
               k++;
            }
         }
   	  
         /* write the Example on another file */
         ExampleWrite(newE, exampleOut);
	  

         ExampleFree(e);
         e = ExampleRead(exampleIn, gEs);
      }
      ExampleFree(newE);
      fclose(exampleIn);
      printf("size %f\n", count);	
      fclose(exampleOut);
   }

   
   /* Free all the memory */
   /****************************************/
   /* Cannot free the double space allocate */
   for(i = 0; i < ExampleSpecGetNumClasses(gEs); i++)
     {
       for(j = 0; j < ExampleSpecGetNumAttributes(gEs); j++)
	 {
	   MFreePtr(prob[i][j]);
	 }
     }
   
   for(i = 0; i < ExampleSpecGetNumClasses(gEs); i++)
     {
        MFreePtr(prob[i]);
        MFreePtr(mostCommonValue[i]);
     }
     
   


   MFreePtr(prob);
   MFreePtr(mostCommonValue);
   MFreePtr(unknownCount);
   MFreePtr(fillMode);

   return 0;
}
	  

