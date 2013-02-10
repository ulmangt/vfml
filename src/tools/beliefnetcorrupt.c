#include "vfml.h"
#include <stdio.h>
#include <string.h>
#include <sys/times.h>
#include <time.h>
#include <math.h>

char  *gBeliefNetFile    = "DF.bif";
char  *gOutputFile       = "DFOut.bif";
char  *gTargetDirectory  = ".";

int   gNumChanges           = 4;
int   gMaxParentsPerNode    = -1;
long  gSeed = -1;
int   gStdout = 0;

int   gCorruptParameters    = 0;
double gEpsilon             = 0.01;

int   gRandomLinks          = 0;
int   gMakeRandom           = 0;

int  gStartFromEmpty = 0;

//static char *_AllocateString(char *first) {
//   char *second = MNewPtr(sizeof(char) * (strlen(first) + 1));
//
//   strcpy(second, first);
//
//   return second;
//}

static void _processArgs(int argc, char *argv[]) {
   int i;

   /* HERE on the ones that use the next arg make sure it is there */
   for(i = 1 ; i < argc ; i++) {
      if(!strcmp(argv[i], "-f")) {
         gOutputFile = argv[i+1];
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-bnf")) {
         gBeliefNetFile = argv[i+1];
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-h")) {
         printf("Generate a synthetic dataset\n");
         printf("   -f <file name for output> (default DFOut.bif)\n");
         printf("   -bnf <file containing belief net> (default DF.bif)\n");
         printf("   -stdout  output the new net to stdout (default to <stem>.data)\n");
         printf("   -startFromEmpty  Remove all links from net before making any changes (default start from the net the way it is)\n");
         printf("   -numChanges <num>  Randomly add 2*num, then remove 2*num, then reverse 2*num links (default 4)\n");
         printf("   -maxParentsPerNode <num>  Limit each node to <num> parents (default no max).\n");
         printf("   -epsilon <epsilon>  Corrupts parameters by epsilon (then normalize), does not change structure (default do structure)\n");
         printf("   -random <num>  Random starting net with <num> links (default use numChanges).\n");
         printf("   -seed <random seed> (default to random)\n");
         printf("   -target <dir> Set the output directory (default '.')\n");
         printf("   -v increase the message level\n");
         exit(0);
      } else if(!strcmp(argv[i], "-seed")) {
         sscanf(argv[i+1], "%lu", &gSeed);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-v")) {
         DebugSetMessageLevel(DebugGetMessageLevel() + 1);
      } else if(!strcmp(argv[i], "-stdout")) {
         gStdout = 1;
      } else if(!strcmp(argv[i], "-target")) {
         gTargetDirectory = argv[i+1];
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-startFromEmpty")) {
         gStartFromEmpty = 1;
      } else if(!strcmp(argv[i], "-maxParentsPerNode")) {
         sscanf(argv[i+1], "%d", &gMaxParentsPerNode);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-epsilon")) {
         sscanf(argv[i+1], "%lf", &gEpsilon);
         gCorruptParameters = 1;
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-numChanges")) {
         sscanf(argv[i+1], "%d", &gNumChanges);
         /* ignore the next argument */
         i++;
      } else if(!strcmp(argv[i], "-random")) {
         gMakeRandom = 1;
         sscanf(argv[i+1], "%d", &gRandomLinks);
         /* ignore the next argument */
         i++;
      } else {
         printf("unknown argument '%s', use -h for help\n", argv[i]);
         exit(0);
      }
   }
}


static BeliefNet _readBN(void) {
   char fileName[255];

   sprintf(fileName, "%s", gBeliefNetFile);

   return BNReadBIF(fileName);
}

static void _removeAllLinks(BeliefNet bn) {
   BeliefNetNode bnn;
   int i;


   for(i = 0 ; i < BNGetNumNodes(bn) ; i++) {
      bnn = BNGetNodeByID(bn, i);

      BNNodeFreeCPT(bnn);

      while(BNNodeGetNumParents(bnn) > 0) {
         BNNodeRemoveParent(bnn, BNNodeGetNumParents(bnn) - 1);
      }

      BNNodeInitCPT(bnn);
   }   
}

static void _writeBNStats(BeliefNet bn) {
   BeliefNetNode bnn;
   int i, maxValues, maxCPTEntries, maxParents, numParents, numCPTEntries;

   DebugMessage(1, 1, "Belief Net Stats for %s:\n", gBeliefNetFile);
   DebugMessage(1, 1, "   Num Nodes:\t\t%d\n", BNGetNumNodes(bn));

   maxValues = maxCPTEntries = maxParents = numParents = numCPTEntries = 0;

   for(i = 0 ; i < BNGetNumNodes(bn) ; i++) {
      bnn = BNGetNodeByID(bn, i);

      maxValues = max(maxValues, BNNodeGetNumValues(bnn));
      maxParents = max(maxParents, BNNodeGetNumParents(bnn));
      numParents += BNNodeGetNumParents(bnn);
      maxCPTEntries = max(maxCPTEntries, 
                        BNNodeGetNumCPTRows(bnn) * BNNodeGetNumValues(bnn));
      numCPTEntries +=  BNNodeGetNumCPTRows(bnn) * BNNodeGetNumValues(bnn);
   }

   DebugMessage(1, 1, "   Max Values:\t\t%d\n", maxValues);
   DebugMessage(1, 1, "   Num Parents:\t\t%d\n", numParents);
   DebugMessage(1, 1, "   Max Parents:\t\t%d\n", maxParents);
   DebugMessage(1, 1, "   Num CPT Entries:\t%d\n", numCPTEntries);
   DebugMessage(1, 1, "   Max CPT Entries:\t%d\n", maxCPTEntries);
}

static void _CorruptParametersBy(BeliefNet bn, double epsilon) {
   BeliefNetNode bnn;
   int i, j, k;
   int numRows;
   double probSum, sign, prob;
   
   for(i = 0 ; i < BNGetNumNodes(bn) ; i++) {
      bnn = BNGetNodeByID(bn, i);
      numRows = BNNodeGetNumCPTRows(bnn);
      for(j = 0 ; j < numRows ; j++) {
         probSum = 0.0;

         for(k = 0 ; k < BNNodeGetNumValues(bnn) ; k++) {
            sign = RandomRange(0, 1) ? 1.0 : -1.0;

            prob = BNNodeGetCPIndexed(bnn, j, k);

            prob += RandomDouble() * epsilon * sign;
            prob = min(prob, 1.0);
            prob = max(prob, 0.0);

            BNNodeSetCPIndexed(bnn, j, k, prob);

            probSum += prob;
         }

         for(k = 0 ; k < BNNodeGetNumValues(bnn) ; k++) {
            BNNodeSetCPIndexed(bnn, j, k, 
               BNNodeGetCPIndexed(bnn, j, k) * (1.0 / probSum));
         }
      }
   }
}

static int _AttemptToRemoveRandomLink(BeliefNet bn) {
   BeliefNetNode srcNode;
   int numLinks, i, linkToRemove, done;

   numLinks = 0;

   BNFlushStructureCache(bn);

   for(i = 0 ; i < BNGetNumNodes(bn) ; i++) {
      srcNode = BNGetNodeByID(bn, i);
      numLinks += BNNodeGetNumParents(srcNode);
   }

   if(numLinks == 0) {
      return 0;
   }

   linkToRemove = RandomRange(0, numLinks - 1);

   done = 0;
   for(i = 0 ; i < BNGetNumNodes(bn) && !done ; i++) {
      srcNode = BNGetNodeByID(bn, i);
      if(linkToRemove >= BNNodeGetNumParents(srcNode)) {
         linkToRemove -= BNNodeGetNumParents(srcNode);
      } else {
         BNNodeFreeCPT(srcNode);
         BNNodeRemoveParent(srcNode, linkToRemove);
         BNNodeInitCPT(srcNode);
         done = 1;
      }
   }

   return 1;
}

static int _AttemptToReverseRandomLink(BeliefNet bn) {
   BeliefNetNode srcNode, dstNode;
   int numLinks, i, linkToRemove, done, numTries, foundLink;

   numLinks = 0;

   for(i = 0 ; i < BNGetNumNodes(bn) ; i++) {
      srcNode = BNGetNodeByID(bn, i);
      numLinks += BNNodeGetNumParents(srcNode);
   }

   if(numLinks == 0) {
      return 0;
   }

   done = 0;
   numTries = 100;
   while(numTries-- && !done) {
      BNFlushStructureCache(bn);
      linkToRemove = RandomRange(0, numLinks - 1);

      foundLink = 0;
      for(i = 0 ; i < BNGetNumNodes(bn) && !foundLink ; i++) {
         srcNode = BNGetNodeByID(bn, i);
         if(linkToRemove >= BNNodeGetNumParents(srcNode)) {
            linkToRemove -= BNNodeGetNumParents(srcNode);
         } else {
            /* try the reversal, if it doesn't work put it back */
            dstNode = BNNodeGetParent(srcNode, linkToRemove);

            BNNodeFreeCPT(srcNode);
            BNNodeFreeCPT(dstNode);
            BNNodeRemoveParent(srcNode, linkToRemove);
            BNNodeAddParent(dstNode, srcNode);

            if(BNHasCycle(bn)) {
               BNNodeRemoveParent(dstNode,
                   BNNodeLookupParentIndex(dstNode, srcNode));
               BNNodeAddParent(srcNode, dstNode);
            } else {
               done = 1;
            }

            BNNodeInitCPT(srcNode);
            BNNodeInitCPT(dstNode);
            foundLink = 1;
         }
      }
   }

   return done;
}

static int _AttemptToAddRandomLink(BeliefNet bn) {
   BeliefNetNode srcNode, dstNode;
   int done, tries;

   tries = 100;
   done = 0;
   while(!done && tries--) {
      BNFlushStructureCache(bn);
      dstNode = BNGetNodeByID(bn, RandomRange(0, BNGetNumNodes(bn) - 1));

      if((gMaxParentsPerNode == -1 ||
                BNNodeGetNumParents(dstNode) + 1 <= gMaxParentsPerNode)) {

         do {
            srcNode = BNGetNodeByID(bn, RandomRange(0, BNGetNumNodes(bn) - 1));
         } while(dstNode == srcNode);

         if(BNNodeLookupParentIndex(dstNode, srcNode) == -1 &&
                BNNodeLookupParentIndex(srcNode, dstNode) == -1) {
            BNNodeFreeCPT(dstNode);
            BNNodeAddParent(dstNode, srcNode);
            BNNodeInitCPT(dstNode);

            if(!BNHasCycle(bn)) {
               done = 1;
            } else {
               BNNodeFreeCPT(dstNode);
               BNNodeRemoveParent(dstNode, 
                        BNNodeLookupParentIndex(dstNode, srcNode));
               BNNodeInitCPT(dstNode);
            }           
         }
      }
   }
   return done;
}


int main(int argc, char *argv[]) {
   int tries, changes;
   BeliefNet bn, outBn;
   FILE *out;

   _processArgs(argc, argv);

   /* randomize the random # generator */
   RandomInit();
   /* seed for the data */
   if(gSeed != -1) {
      RandomSeed(gSeed);
   } else {
      gSeed = RandomRange(1, 30000);
      RandomSeed(gSeed);
   }

   bn = _readBN();

   if(gStartFromEmpty) {
      _removeAllLinks(bn);
   }

   if(DebugGetMessageLevel() > 1) {
      _writeBNStats(bn);
   }

   outBn = BNClone(bn);

   if(gCorruptParameters) {
      _CorruptParametersBy(outBn, gEpsilon);
   } else if(gMakeRandom) {
      _removeAllLinks(bn);
      changes = 0;
      tries = 100;
      while(tries-- && changes < gRandomLinks) {
         changes += _AttemptToAddRandomLink(outBn);
      }
   } else {
      changes = 0;
      tries = 100;
      while(tries-- && changes < gNumChanges * 2) {
         changes += _AttemptToAddRandomLink(outBn);
      }

      changes = 0;
      tries = 100;
      while(tries-- && changes < gNumChanges * 2) {
         changes += _AttemptToRemoveRandomLink(outBn);
      }

      changes = 0;
      tries = 100;
      while(tries-- && changes < gNumChanges * 2) {
         changes += _AttemptToReverseRandomLink(outBn);
      }
   }

   _writeBNStats(outBn);
   if(gStdout) {
      BNWriteBIF(outBn, stdout);
   } else {
      out = fopen(gOutputFile, "w");
      BNWriteBIF(outBn, out);
      fclose(out);
   }

   return 0;
}
