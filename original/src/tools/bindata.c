#include "vfml.h"
#include <stdio.h>
#include <math.h>

//#include <string.h>

char  *gFileStem = "DF";
char  *gOutputStem = "DF-out";
int   gNumBins = 10;
int   gDoGaussianBins = 0;
int   gConvertTest = 0;
int   gSampleSize = -1;
int   gMessageLevel = 0;
char  *gTargetDirectory = ".";
char  *gSourceDirectory = ".";
long  gSeed = -1;

static void _printUsage(char  *name) {
   printf("%s: Turns continuous attributes into discrete ones via equal-with binning\n\tUses two passes over the data set from disk, but not too much RAM.\n", name);

   printf("-f <filestem>\tSet the name of the dataset (default DF)\n");
   printf("-fout <filestem>\tSet the name of the output dataset (default DF-out)\n");
   printf("-target <dir>\tSet the output directory (default '.')\n");
   printf("-test\t\tAlso converts the test set (but only uses .data to pick bin breaks)\n");
   printf("-source <dir>\tSet the source data directory (default '.')\n");
   printf("-bins <n>\tSets the number of bins (default 10)\n");
   printf("-gaussian\tPick bin boundaries by fitting a Gaussian and making even probability bins\n");
   printf("-samples <n>\tUse the first <n> examples to pick bin boundaries (default whole train set)\n");

   printf("-v\t\tCan be used multiple times to increase the debugging output\n");
}

static void _processArgs(int argc, char *argv[]) {
   int i;

   /* HERE on the ones that use the next arg make sure it is there */
   for(i = 1 ; i < argc ; i++) {
      if(!strcmp(argv[i], "-f")) {
         gFileStem = argv[i+1];
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-fout")) {
         gOutputStem = argv[i+1];
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-target")) {
         gTargetDirectory = argv[i+1];
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-test")) {
         gConvertTest = 1;
      } else if(!strcmp(argv[i], "-gaussian")) {
         gDoGaussianBins = 1;
         //DebugError(1, "Gaussian bins are not implemented yet.");
      } else if(!strcmp(argv[i], "-source")) {
         gSourceDirectory = argv[i+1];
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-bins")) {
         sscanf(argv[i+1], "%d", &gNumBins);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-samples")) {
         sscanf(argv[i+1], "%d", &gSampleSize);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-v")) {
         gMessageLevel++;
         DebugSetMessageLevel(gMessageLevel);
      } else if(!strcmp(argv[i], "-h")) {
         _printUsage(argv[0]);
         exit(0);
      } else {
         printf("Unknown argument: %s.  use -h for help\n", argv[i]);
         exit(0);
      }
   }

   if(gMessageLevel >= 1) {
      printf("File stem: %s\n", gFileStem);
      printf("Target: %s\n", gTargetDirectory);
      printf("Source: %s\n", gSourceDirectory);
      printf("Bins: %.d\n", gNumBins);
      printf("Seed: %ld\n", gSeed);
   }
}

static char *_AllocateString(char *first) {
   char *second = MNewPtr(sizeof(char) * (strlen(first) + 1));

   strcpy(second, first);

   return second;
}

static FloatListPtr *MakeUniformBoundaries(ExampleSpecPtr es, float *min,
                                 float *max, int *init) {
   int i, j;
   float binWidth;
   float nextBound;
   FloatListPtr *boundaries;
   FloatListPtr curBound;


   boundaries = (FloatListPtr *)MNewPtr(sizeof(FloatListPtr *) * 
                                       ExampleSpecGetNumAttributes(es));
   for(i = 0 ; i < ExampleSpecGetNumAttributes(es) ; i++) {
      if(ExampleSpecIsAttributeContinuous(es, i)) {
         curBound = FLNew();
         if(init[i]) {
            binWidth = (max[i] - min[i]) / (float)gNumBins;

            nextBound = min[i] + binWidth;
            for(j = 0 ; j < (gNumBins - 1) ; j++) {
               FLAppend(curBound, nextBound);
               nextBound += binWidth;
            }
         } else {
            FLAppend(curBound, 1.0);
         }
         boundaries[i] = curBound;
      }
   }
   return boundaries;
}

const static double _NormInvP[] = {0, 0.0001, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 0.9999, 1.0, -1};
const static double _NormInvV[] = {-6.362035677, -3.719090272, -1.281551939, -0.841621042, -0.524400458, -0.253347241, 0.00000, 0.253347241, 0.524400458, 0.841621042, 1.281551939, 3.719090272, 6.362035677, 8};

static double _LookupNormInv(double p) {
   int i;
   int scale;

   i = 0;
   while(_NormInvP[i] != -1) {
      if(p == _NormInvP[i]) {
         return _NormInvV[i];
      } else if(p <= _NormInvP[i+1]) {
         scale = (p - _NormInvP[i]) / (_NormInvP[i+1] - _NormInvP[i]);
         return _NormInvV[i] + (scale * fabs((_NormInvV[i+1] - _NormInvV[i])));
      }
      i++;
   }
   /* this had better not happen */
   DebugMessage(1, 0, "Problem in _LookupNormInv()\n");
   return 8.0;
}

static FloatListPtr *MakeGaussianBoundaries(ExampleSpecPtr es, 
                                                StatTracker *statTrackers) {
   int i, j;
   FloatListPtr *boundaries;
   FloatListPtr curBound;
   double *values;
   StatTracker curTracker;
   double stdev, mean;

   values = MNewPtr(sizeof(double) * (gNumBins - 1));

   for(i = 0 ; i < gNumBins - 1 ; i++) {
      values[i] = _LookupNormInv((i + 1) * (1.0 / (float)gNumBins));
   }

   boundaries = (FloatListPtr *)MNewPtr(sizeof(FloatListPtr *) * 
                                       ExampleSpecGetNumAttributes(es));
   for(i = 0 ; i < ExampleSpecGetNumAttributes(es) ; i++) {
      if(ExampleSpecIsAttributeContinuous(es, i)) {
         curBound = FLNew();
         curTracker = statTrackers[i];
         if(StatTrackerGetNumSamples(curTracker) > 0) {
            mean = StatTrackerGetMean(statTrackers[i]);
            stdev = StatTrackerGetStdev(statTrackers[i]);
            for(j = 0 ; j < gNumBins - 1 ; j++) {
               FLAppend(curBound, mean + (stdev * values[j]));
            }
         } else {
            FLAppend(curBound, 1.0);
         }
         boundaries[i] = curBound;
      }
   }

   return boundaries;
}


static ExampleSpecPtr _MakeNewSpecFromBoundaries(ExampleSpecPtr es, 
                           FloatListPtr *boundaries) {
   int i, j;
   char name[255];
   FloatListPtr curBound;
   ExampleSpecPtr newSpec = ExampleSpecNew();

   for(i = 0 ; i < ExampleSpecGetNumClasses(es) ; i++) {
      ExampleSpecAddClass(newSpec, 
            _AllocateString(ExampleSpecGetClassValueName(es, i)));
   }

   for(i = 0 ; i < ExampleSpecGetNumAttributes(es) ; i++) {
      if(ExampleSpecIsAttributeDiscrete(es, i)) {
         ExampleSpecAddDiscreteAttribute(newSpec, 
            _AllocateString(ExampleSpecGetAttributeName(es, i)));

         for(j = 0 ; j < ExampleSpecGetAttributeValueCount(es, i) ; j++) {
            ExampleSpecAddAttributeValue(newSpec, i,
               _AllocateString(ExampleSpecGetAttributeValueName(es, i, j)));
         }
      } else if(ExampleSpecIsAttributeContinuous(es, i)) {
         curBound = boundaries[i];

         ExampleSpecAddDiscreteAttribute(newSpec, 
               _AllocateString(ExampleSpecGetAttributeName(es, i)));

         if(FLLength(curBound) == 0) {
            sprintf(name, "BUSTED");
            ExampleSpecAddAttributeValue(newSpec, i, _AllocateString(name));
            DebugMessage(1, 0, "Attribute %d has no binning.\n", i);
         } else {
            sprintf(name, "nInf-%.10g", FLIndex(curBound,0));
            ExampleSpecAddAttributeValue(newSpec, i, _AllocateString(name));
            for(j = 1 ; j < FLLength(curBound) ; j++) {
               sprintf(name, "%.10g-%.10g", FLIndex(curBound, j-1), 
                                  FLIndex(curBound,j));
               ExampleSpecAddAttributeValue(newSpec, i, _AllocateString(name));
            }
            sprintf(name, "%.10g-pInf", FLIndex(curBound, 
                                             FLLength(curBound) - 1));
            ExampleSpecAddAttributeValue(newSpec, i, _AllocateString(name));
         }
      } else if(ExampleSpecIsAttributeIgnored(es, i)) {
         DebugMessage(1, 0, "There is an ignored attribute in the orginal spec.\n");
         DebugMessage(1, 0, "  There is a good chance this will crash bindata.\n");
         ExampleSpecAddDiscreteAttribute(newSpec, 
            _AllocateString(ExampleSpecGetAttributeName(es, i)));
         ExampleSpecIgnoreAttribute(newSpec, i);
      } else {
         DebugError(1, "Unsupported attribute type\n");
      }
   }
   return newSpec;
}


int main(int argc, char *argv[]) {
   char fileName[255];
   int i, j, count;
   FILE *exampleIn, *names;
   ExampleSpecPtr es, newSpec;
   ExamplePtr e, newE;
   FILE *outputData;
   //FILE *outputTest;
   float *max;
   float *min;
   int   *init;
   StatTracker *statTrackers;

   int value;
   FloatListPtr *boundaries;
   FloatListPtr curBound;

   _processArgs(argc, argv);

   /* set up the random seed */
   RandomInit();
   if(gSeed != -1) {
      RandomSeed(gSeed);
   }

   /* read the spec file */
   sprintf(fileName, "%s/%s.names", gSourceDirectory, gFileStem);
   /* TODO check that the file exists */
   es = ExampleSpecRead(fileName);
   DebugError(es == 0, "Unable to open the .names file");

   /* get the ranges for each continuous variable */
   sprintf(fileName, "%s/%s.data", gSourceDirectory, gFileStem);
   exampleIn = fopen(fileName, "r");
   DebugError(exampleIn == 0, "Unable to open .data file");

   max = MNewPtr(sizeof(float) * ExampleSpecGetNumAttributes(es));
   min = MNewPtr(sizeof(float) * ExampleSpecGetNumAttributes(es));
   init = MNewPtr(sizeof(int) * ExampleSpecGetNumAttributes(es));
   statTrackers = MNewPtr(sizeof(StatTracker *) * 
                                  ExampleSpecGetNumAttributes(es));

   for(i = 0 ; i < ExampleSpecGetNumAttributes(es) ; i++) {
      init[i] = 0;
      statTrackers[i] = StatTrackerNew();
   }

   DebugMessage(1, 1, "Scanning data set...\n");
   e = ExampleRead(exampleIn, es);
   count = 0;
   while(e != 0 && ((gSampleSize == -1) || count < gSampleSize)) {
      count++;
      for(i = 0 ; i < ExampleSpecGetNumAttributes(es) ; i++) {
         if(ExampleSpecIsAttributeContinuous(es, i)) {
            if(!ExampleIsAttributeUnknown(e, i)) {
               StatTrackerAddSample(statTrackers[i], 
                               ExampleGetContinuousAttributeValue(e, i));
               if(init[i] == 0) {
                  init[i] = 1;
                  max[i] = min[i] = ExampleGetContinuousAttributeValue(e, i);
               } else {
                  if(max[i] < ExampleGetContinuousAttributeValue(e, i)) {
                     max[i] = ExampleGetContinuousAttributeValue(e, i);
                  }
                  if(min[i] > ExampleGetContinuousAttributeValue(e, i)) {
                     min[i] = ExampleGetContinuousAttributeValue(e, i);
                  }
               }
            }
         }
      }

      ExampleFree(e);
      e = ExampleRead(exampleIn, es);
   }
   fclose(exampleIn);

   if(gDoGaussianBins) {
      boundaries = MakeGaussianBoundaries(es, statTrackers);
   } else {
      boundaries = MakeUniformBoundaries(es, min, max, init);
   }


   /* output the names file */
   newSpec = _MakeNewSpecFromBoundaries(es, boundaries);

   /* TODO check that the target directory exists */
   DebugMessage(1, 1, "Outputting spec file...\n");

   sprintf(fileName, "%s/%s.names", gTargetDirectory, gOutputStem);
   names = fopen(fileName, "w");
   DebugError(names == 0, "Unable to open target names file for output.");

   ExampleSpecWrite(newSpec, names);
   fclose(names);


   /* Output the new data */
   /* TODO check that the target directory exists */
   sprintf(fileName, "%s/%s.data", gTargetDirectory, gOutputStem);
   outputData = fopen(fileName, "w");

   DebugError(outputData == 0, "Unable to open output file.");

   sprintf(fileName, "%s/%s.data", gSourceDirectory, gFileStem);
   exampleIn = fopen(fileName, "r");
   DebugError(exampleIn == 0, "Unable to open .data file");

   DebugMessage(1, 1, "Generating new train set...\n");
   e = ExampleRead(exampleIn, es);
   while(e != 0) {
      newE = ExampleNew(newSpec);

      ExampleSetClass(newE, ExampleGetClass(e));

      for(i = 0 ; i < ExampleSpecGetNumAttributes(newSpec) ; i++) {
         if(ExampleIsAttributeUnknown(e, i)) {
            ExampleSetAttributeUnknown(newE, i);
         } else if(ExampleSpecIsAttributeDiscrete(es, i)) {
            ExampleSetDiscreteAttributeValue(newE, i,
                   ExampleGetDiscreteAttributeValue(e, i));
         } else {
            curBound = boundaries[i];
            if(FLLength(curBound) == 0) {
               value = 0;
            } else {
               value = -1;
               for(j = 0 ; j < FLLength(curBound) && value == -1; j++) {
                  /* HERE EFF could maybe be a binary search */
                  if(ExampleGetContinuousAttributeValue(e, i) < 
                                                        FLIndex(curBound, j)) {
                     /* find the first bb > the eg val */
                     value = j;
                  }
               }
              
               if(value == -1) {
                  /* no boun > the eg val, so it is in the last bin */
                  value = FLLength(curBound);
               }

            }
            //printf("a %d v %d\n", i, value);
            ExampleSetDiscreteAttributeValue(newE, i, value);
         }
      }

      ExampleWrite(newE, outputData);

      ExampleFree(newE);
      ExampleFree(e);
      e = ExampleRead(exampleIn, es);
   }
   fclose(exampleIn);
   fclose(outputData);


   /* Now do the test set if requested */

   if(gConvertTest) {
      DebugMessage(1, 1, "Generating new test set...\n");
      sprintf(fileName, "%s/%s.test", gSourceDirectory, gFileStem);
      exampleIn = fopen(fileName, "r");
      DebugError(exampleIn == 0, "Unable to open input .test file");
   
      /* TODO check that the target directory exists */
      sprintf(fileName, "%s/%s.test", gTargetDirectory, gOutputStem);
      outputData = fopen(fileName, "w");

      DebugError(outputData == 0, "Unable to open output .test file.");

      e = ExampleRead(exampleIn, es);
      count = 0;
      while(e != 0) {
         count++;
         //printf("%d\n", count);
         //ExampleWrite(e, stdout);

         newE = ExampleNew(newSpec);
         ExampleSetClass(newE, ExampleGetClass(e));

         for(i = 0 ; i < ExampleSpecGetNumAttributes(newSpec) ; i++) {
            if(ExampleIsAttributeUnknown(e, i)) {
               ExampleSetAttributeUnknown(newE, i);
            } else if(ExampleSpecIsAttributeDiscrete(es, i)) {
               ExampleSetDiscreteAttributeValue(newE, i,
                      ExampleGetDiscreteAttributeValue(e, i));
            } else {
               curBound = boundaries[i];
               if(FLLength(curBound) == 0) {
                  value = 0;
               } else {
                  value = -1;
                  for(j = 0 ; j < FLLength(curBound) && value == -1; j++) {
                     /* HERE EFF could maybe be a binary search */
                     if(ExampleGetContinuousAttributeValue(e, i) < 
                                                        FLIndex(curBound, j)) {
                        /* find the first bb > the eg val */
                        value = j;
                     }
                  }
              
                  if(value == -1) {
                     /* no boun > the eg val, so it is in the last bin */
                     value = FLLength(curBound);
                  }
               }
               ExampleSetDiscreteAttributeValue(newE, i, (int)value);
            }
         }

         ExampleWrite(newE, outputData);

         ExampleFree(newE);
         ExampleFree(e);
         e = ExampleRead(exampleIn, es);
      }
      fclose(exampleIn);
      fclose(outputData);
   }

   return 0;
}

