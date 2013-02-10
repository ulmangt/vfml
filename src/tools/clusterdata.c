#include "vfml.h"
#include <stdio.h>
#include <string.h>
#include <sys/times.h>
#include <time.h>
#include <math.h>

char  *gFileStem         = "DF";
int   gNumDiscDimensions = 0;
int   gNumContDimensions = 5;
int   gNumClusters       = 5;
int   gInfiniteStream    = 0;
char  *gTargetDirectory  = ".";

float gStdev             = .1;

unsigned long  gNumTrainExamples = 10000;

long  gSeed = -1;
long  gConceptSeed = -1;
int   gMessageLevel = 0;
int   gStdout = 0;

/* Some hack globals */
ExampleSpecPtr      gEs; /* will be set to the spec for the run */
VoidAListPtr        gCentroids;
float               gMinDistance;

static char *_AllocateString(char *first) {
   char *second = MNewPtr(sizeof(char) * (strlen(first) + 1));

   strcpy(second, first);

   return second;
}

static void _processArgs(int argc, char *argv[]) {
   int i;

   /* HERE on the ones that use the next arg make sure it is there */
   for(i = 1 ; i < argc ; i++) {
      if(!strcmp(argv[i], "-f")) {
         gFileStem = argv[i+1];
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-h")) {
         printf("Generate a synthetic data set by sampling from a mixture of Gaussians.\n");
         printf("   -f <stem name> (default DF)\n");
         //printf("   -discrete <number of discrete dimensions> (default 0)\n");
         printf("   -continuous <number of continous dimenstions> (default 5)\n");
         printf("   -clusters <number of clusters> (default 5)\n");
         printf("   -train <size of training set> (default 10000)\n");
         printf("   -infinite\t Generate an infinite stream of training examples,\n\t overrides -train flags, only makes sense with -stdout (default off)\n");
         printf("   -stdout  output the trainset to stdout (default to <stem>.data)\n");
         printf("   -stdev <the standard deviation on each dimension> (default 0.1)\n");

         printf("   -conceptSeed <the multiplier for the concept seed> (default to random)\n");
         printf("   -seed <random seed> (default to random)\n");
         printf("   -target <dir> Set the output directory (default '.')\n");
         printf("   -v increase the message level\n");
         exit(0);
      } else if(!strcmp(argv[i], "-discrete")) {
         sscanf(argv[i+1], "%d", &gNumDiscDimensions);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-continuous")) {
         sscanf(argv[i+1], "%d", &gNumContDimensions);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-clusters")) {
         sscanf(argv[i+1], "%d", &gNumClusters);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-train")) {
         sscanf(argv[i+1], "%lu", &gNumTrainExamples);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-stdev")) {
         sscanf(argv[i+1], "%f", &gStdev);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-seed")) {
         sscanf(argv[i+1], "%lu", &gSeed);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-conceptSeed")) {
         sscanf(argv[i+1], "%ld", &gConceptSeed);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-v")) {
         gMessageLevel++;
      } else if(!strcmp(argv[i], "-stdout")) {
         gStdout = 1;
      } else if(!strcmp(argv[i], "-infinite")) {
         gInfiniteStream = 1;
      } else if(!strcmp(argv[i], "-target")) {
         gTargetDirectory = argv[i+1];
         /* ignore the next argument */
         i++;
      } else {
         printf("unknown argument '%s', use -h for help\n", argv[i]);
         exit(0);
      }
   }
}


static void _makeSpec(void) {

   char tmpStr[255];
   int i, j, valueCount, num;
   char fileNames[255];
   FILE *fileOut;

   gEs = ExampleSpecNew();

   for(i = 0 ; i < gNumClusters ; i++) {
      sprintf(tmpStr, "cluster-%d", i);
      ExampleSpecAddClass(gEs, _AllocateString(tmpStr));
   }

   for(i = 0 ; i < gNumDiscDimensions ; i++) {
      sprintf(tmpStr, "d-dimension-%d", i);
      num = ExampleSpecAddDiscreteAttribute(gEs, _AllocateString(tmpStr));

      valueCount = 2;
      for(j = 0 ; j < valueCount ; j++) {
         sprintf(tmpStr, "%d-%d", i, j);
         ExampleSpecAddAttributeValue(gEs, num, _AllocateString(tmpStr));
      }
   }

   for(i = 0 ; i < gNumContDimensions ; i++) {
      sprintf(tmpStr, "c-dimension-%d", i);
      num = ExampleSpecAddContinuousAttribute(gEs, _AllocateString(tmpStr));
   }

   sprintf(fileNames, "%s/%s.names", gTargetDirectory, gFileStem);
   fileOut = fopen(fileNames, "w");
   ExampleSpecWrite(gEs, fileOut);
   fclose(fileOut);
}

static void _makeData(void) {
   char fileNames[255];
   FILE *fileOut;
   ExamplePtr e, center;
   double value;
   int cluster;
   long i, j;


   if(!gStdout) {
      sprintf(fileNames, "%s/%s.data", gTargetDirectory, gFileStem);
      fileOut = fopen(fileNames, "w");
   } else {
      fileOut = stdout;
   }

   for(i = 0 ; i < gNumTrainExamples || gInfiniteStream ; i++) {
      e = ExampleNew(gEs);

      /* pick a random cluster for the point */
      cluster = RandomRange(0, VLLength(gCentroids) - 1);
      center = VALIndex(gCentroids, cluster);
      ExampleSetClass(e, cluster);
      
      /* now set the example's attributes based on the clusters & stdev*/
      for(j = 0 ; j < ExampleSpecGetNumAttributes(gEs) ; j++) {
         if(ExampleSpecIsAttributeDiscrete(gEs, j)) {
            ExampleSetDiscreteAttributeValue(e, j,
                      ExampleGetDiscreteAttributeValue(center, j));
         } else {
            value = RandomGaussian(ExampleGetContinuousAttributeValue(center,
                           j), gStdev);
            //if(value < 0.0) { value = 0.0; }
            //if(value > 1.0) { value = 1.0; }
            ExampleSetContinuousAttributeValue(e, j, value);

         }
      }
//      fprintf(fileOut, "distance to cluster %lf:  ", ExampleDistance(e, center));
      ExampleWrite(e, fileOut);
      ExampleFree(e);
   }

   if(!gStdout) {
      fclose(fileOut);
   }
}

static void _makeCentroids(void) {
   int i,j;
   int tooClose;
   int remainingTries;
   ExampleGeneratorPtr eg;
   ExamplePtr e, center;
   float thisDistance;
   char fileName[255];
   FILE *fileOut;

   eg = ExampleGeneratorNew(gEs, gConceptSeed);
   gCentroids = VALNew();

   if(gMessageLevel > 0) {
      printf("Generate clusters");
      fflush(stdout);
   }

   for(i = 0 ; i < gNumClusters ; i++) {
      if(gMessageLevel > 1) {
         printf(".");
         fflush(stdout);
      }

      tooClose = 1;
      remainingTries = 1000;
      while(tooClose && remainingTries > 0) {
         remainingTries--;

         /* pick a point for the new center */
         e = ExampleGeneratorGenerate(eg);

         /* scale each continuous dimension into the target space */
         for(j = 0 ; j < ExampleGetNumAttributes(e) ; j++) {
	   if(ExampleSpecIsAttributeContinuous(gEs, j)) {
	     ExampleSetContinuousAttributeValue(e, j,
               (2 * gStdev) + (ExampleGetContinuousAttributeValue(e, j) * 
                                (1 - (4*gStdev))));
           }
         }

         /* make sure it isn't too close to any existing center */
         tooClose = 0;
         for(j = 0 ; j < VALLength(gCentroids) && !tooClose ; j++) {
            center = VALIndex(gCentroids, j);
            thisDistance = ExampleDistance(e, center);

            if(gMessageLevel > 2) {
               printf("Distance %f min %f\n", thisDistance, 
                               gMinDistance);
               fflush(stdout);
            }

            if(thisDistance < gMinDistance) {
               if(gMessageLevel > 1) {
                  printf("Too close, distance %f min %f\n", thisDistance, 
                                  gMinDistance);
                  fflush(stdout);
               }
               tooClose = 1;
            }
         }
      }
      /* use the class label to name the cluster */
      ExampleSetClass(e, i);
      VALAppend(gCentroids, e);
   }

   sprintf(fileName, "%s/%s.test", gTargetDirectory, gFileStem);
   fileOut = fopen(fileName, "w");
   for(i = 0 ; i < VALLength(gCentroids) ; i++) {
      ExampleWrite(VALIndex(gCentroids, i), fileOut);
   }

   fclose(fileOut);
}

static void _printStats(void) {
   char fileNames[255];
   FILE *fileOut;

   /* HERE NOT USED */
   sprintf(fileNames, "%s/%s.stats", gTargetDirectory, gFileStem);
   fileOut = fopen(fileNames, "w");

   fprintf(fileOut, "Num Dimensions: %d discrete & %d continuous\n", gNumDiscDimensions, gNumContDimensions);
   fprintf(fileOut, "Num Clusters %d\n", gNumClusters);

   fprintf(fileOut, "Num train examples %ld\n", gNumTrainExamples);
   fprintf(fileOut, "Example seed %ld\n", gSeed);
   fprintf(fileOut, "Concept seed %ld\n", gConceptSeed);

   fclose(fileOut);
}


int main(int argc, char *argv[]) {

   _processArgs(argc, argv);

   RandomInit();
   /* seed for the concept */
   if(gConceptSeed != -1) {
      RandomSeed(gConceptSeed);
   } else {
      gConceptSeed = RandomRange(1, 30000);
      RandomSeed(gConceptSeed);
   }

   _makeSpec();

   gMinDistance = (sqrt(gNumDiscDimensions + gNumContDimensions) /
                             (float)(gNumClusters * 1)) * gStdev;

   _makeCentroids();

   /* randomize the random # generator again */
   RandomInit();
   /* seed for the data */
   if(gSeed != -1) {
      RandomSeed(gSeed);
   } else {
      gSeed = RandomRange(1, 30000);
      RandomSeed(gSeed);
   }

   _makeData();

   return 0;
}
 
