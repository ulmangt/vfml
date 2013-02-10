#include "vfml.h"
#include <stdio.h>
#include <string.h>
#include <sys/times.h>
#include <time.h>
#include <math.h>

char  *gDataFile         = "DF.data";
char  *gBeliefNetFile    = "DF.bif";
char  *gCompareNetFile   = "";
char  *gTargetDirectory  = ".";
int   gStdin = 0;

int    gDoSmooth         = 0;
double gPriorStrength    = 0;

int   gCompareToNet      = 0;

static void _processArgs(int argc, char *argv[]) {
   int i;

   /* HERE on the ones that use the next arg make sure it is there */
   for(i = 1 ; i < argc ; i++) {
      if(!strcmp(argv[i], "-f")) {
         gDataFile = argv[i+1];
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-bnf")) {
         gBeliefNetFile = argv[i+1];
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-compareWith")) {
         gCompareNetFile = argv[i+1];
         gCompareToNet = 1;
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-h")) {
         printf("Evaluate beliefnets\n");
         printf("   -f <test set file> (default DF.data)\n");
         printf("   -bnf <file containing belief net> (default DF.bif)\n");
         printf("   -compareWith <file containing belief net> (default get ll on datafile)\n");
         printf("   -stdin  get test set from stdin (default to -f's file)\n");
         printf("   -smooth <prior-str> Counts net at <prior-str> samples and adds one additional sample to each CPT entry before testing\n");
         printf("   -v increase the message level\n");
         exit(0);
      } else if(!strcmp(argv[i], "-v")) {
         DebugSetMessageLevel(DebugGetMessageLevel() + 1);
      } else if(!strcmp(argv[i], "-smooth")) {
         gDoSmooth = 1;
         sscanf(argv[i+1], "%lf", &gPriorStrength);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-stdin")) {
         gStdin = 1;
      } else {
         printf("unknown argument '%s', use -h for help\n", argv[i]);
         exit(0);
      }
   }
}


static BeliefNet _readBN(char *name) {
   char fileName[255];

   sprintf(fileName, "%s", name);

   return BNReadBIF(fileName);
}

static int _GetStructuralDifference(BeliefNet bn, BeliefNet cbn) {
   int difference = 0;
   int i, j;
   BeliefNetNode bnn, cbnn;

   difference = 0;
   for(i = 0 ; i < BNGetNumNodes(bn) ; i++) {
      bnn = BNGetNodeByID(bn, i);
      cbnn = BNGetNodeByID(cbn, i);
      for(j = 0 ; j < BNNodeGetNumParents(bnn) ; j++) {
         if(!BNNodeHasParentID(cbnn, BNNodeGetParentID(bnn, j))) {
            difference++;
         }
      }
      for(j = 0 ; j < BNNodeGetNumParents(cbnn) ; j++) {
         if(!BNNodeHasParentID(bnn, BNNodeGetParentID(cbnn, j))) {
            difference++;
         }
      }
   }

   return difference;
}

int main(int argc, char *argv[]) {
   BeliefNet bn, cbn;
   FILE *dataFile;
   ExamplePtr e;
   long numSeen = 0;
   double llSum = 0;

   _processArgs(argc, argv);

   bn = _readBN(gBeliefNetFile);

   if(gDoSmooth) {
      BNSetPriorStrength(bn, gPriorStrength);
      BNSmoothProbabilities(bn, 1);
      if(DebugGetMessageLevel() > 2) {
         BNWriteBIF(bn, stdout);
      }
   }

   if(gCompareToNet) {
      cbn = _readBN(gCompareNetFile);

      if(gDoSmooth) {
         BNSetPriorStrength(cbn, gPriorStrength);
         BNSmoothProbabilities(cbn, 1);
      }

      DebugMessage(1, 0, "Structural Difference: %d\n",
         _GetStructuralDifference(bn, cbn));
      
   } else {
      if(gStdin) {
         dataFile = stdin;
      } else {
         dataFile = fopen(gDataFile, "r");
         DebugError(dataFile == 0, "Unable to open datafile.");
      }

      e = ExampleRead(dataFile, BNGetExampleSpec(bn));
      while(e != 0) {
         numSeen++;

         llSum += BNGetLogLikelihood(bn, e);

         ExampleFree(e);
         e = ExampleRead(dataFile, BNGetExampleSpec(bn));
      }

      DebugMessage(1, 1, "Average Log Likelihood of %s on %s is: ",
              gBeliefNetFile, gDataFile);
      DebugMessage(1, 0, "%lf\n", llSum / (double)numSeen);

      if(!gStdin) {
        fclose(dataFile);
      } 
   }

   return 0;
}
