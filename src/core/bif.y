%{
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "vfml.h"
%}

%{
int biflex(void);
int biferror(const char *);

/* HERE figure out how to give better error messages */
/* BUG needs a \n at the end of the names file */

/* These tmps are allocated at the begining of parsing and then
    used during parsing.  After parsing a
    statement, the associated tmp is added to the appropriate
    global list, and a new tmp is allocated.  Finally, at the
    end of parsing, all the tmps are freed
*/

BeliefNet             bn;
BeliefNetNode         bnn;
BeliefNetNode         currentNode;
int                   gLineNumber;
int                   seenProbabilityEntry;

//IntListPtr            probVarList;
FloatListPtr          floatList;
VoidListPtr           stringList;

void _SetProbsFromTable(BeliefNetNode bnn, FloatListPtr fList);

%}

%union {
   int integer;
   float f;
   char *string;
}


%token <integer> tInteger
%token <f> tFloat
%token <string>  tString

%token tNetwork tVariable tProbability tProperty tVariableType
%token tDiscrete tDefaultValue tTable tEOF

%%

CompilationUnit: NetworkDeclaration GraphItemList;

NetworkDeclaration: tNetwork tString  '{' NetworkContent {
   BNSetName(bn, $2);
};

NetworkContent: PropertyList '}' | '}';

PropertyList: PropertyList Property | Property;

GraphItemList: GraphItemList GraphItem | GraphItem;
GraphItem: VariableDeclaration | ProbabilityDecl;

VariableDeclaration: tVariable tString {bnn = BNNewNode(bn, $2); }
                                                VariableContent;


VariableContent: '{' VariableContentItemList '}';
VariableContentItemList: VariableContentItemList Property |
                         VariableContentItemList VariableDiscrete |
                         VariableDiscrete |
                          Property;

VariableDiscrete: tVariableType tDiscrete '[' tInteger ']' '{' SList '}' ';' {
   int i;

   for(i = 0 ; i < VLLength(stringList) ; i++) {
      BNNodeAddValue(bnn, (char *)VLIndex(stringList, i));
   }

   VLFree(stringList);
   stringList = VLNew();
};

Property: tProperty {
   //printf("parsed property\n");
};


ProbabilityDecl: tProbability '(' SList ')' ActionInitCPT ProbabilityContent;

ActionInitCPT: {
   int i;
   BeliefNetNode child, parent;


   child = BNLookupNode(bn, (char *)VLIndex(stringList, 0));

   for(i = 1 ; i < VLLength(stringList) ; i++) {
      parent = BNLookupNode(bn, (char *)VLIndex(stringList, i));

      BNNodeAddParent(child, parent);
   }

   BNNodeInitCPT(child);
   currentNode = child;

   for(i = 0 ; i < ILLength(stringList) ; i++) {
      MFreePtr(VLIndex(stringList, i));
   }
   VLFree(stringList);
   stringList = VLNew();
};

ProbabilityContent: '{' { seenProbabilityEntry = 0; } ProbabilityContentItemList '}';


ProbabilityContentItemList: ProbabilityContentItemList ProbabilityContentItem | ProbabilityContentItem;

ProbabilityContentItem: Property | 
                     ProbabilityDefaultEntry |
                     ProbabilityEntry   |
                     ProbabilityTable;

ProbabilityDefaultEntry: tDefaultValue FList ';' {
  if(seenProbabilityEntry) {
      printf("WARNING default seen after a specific entry\n");
   }

   FLFree(floatList);
   floatList = FLNew();
   printf("WARNING parsed default but didn't interperate it\n");
};


ProbabilityEntry: '(' SList ')' FList ';' {
   BeliefNetNode parent;
   int value, i;
   ExamplePtr e = ExampleNew(BNGetExampleSpec(bn));

   seenProbabilityEntry = 1;

   for(i = 0 ; i < VLLength(stringList) ; i++) {
      parent = BNNodeGetParent(currentNode, i);
      value = BNNodeLookupValue(parent, (char *)VLIndex(stringList, i));
      ExampleSetDiscreteAttributeValue(e, BNNodeGetID(parent), value);
   }

   for(i = 0 ; i < BNNodeGetNumValues(currentNode) ; i++){
      ExampleSetDiscreteAttributeValue(e, BNNodeGetID(currentNode), i);
      BNNodeSetCP(currentNode, e, FLIndex(floatList, i));
   }

   /* free all memory and reset lists */
   ExampleFree(e);

   for(i = 0 ; i < ILLength(stringList) ; i++) {
      MFreePtr(VLIndex(stringList, i));
   }
   VLFree(stringList);
   stringList = VLNew();

   FLFree(floatList);
   floatList = FLNew();
};

ProbabilityTable: tTable FList ';' {
   if(seenProbabilityEntry) {
      printf("WARNING table seen after a specific entry\n");
   }

   _SetProbsFromTable(currentNode, floatList);

   FLFree(floatList);
   floatList = FLNew();
};

SList: SList tString {
              VLAppend(stringList, $2);
           } |
           tString { 
              VLAppend(stringList, $1);
           } |
           SList tInteger {
              char tmp[100], *theString;

              //sprintf(tmp, "%d\0", $2);
              sprintf(tmp, "%d", $2);
              theString = MNewPtr(sizeof(char) * (strlen(tmp) + 1));
              sprintf(theString, "%d", $2);
              //sprintf(theString, "%d\0", $2);
              
              VLAppend(stringList, theString);
           } |
           tInteger { 
              char tmp[100], *theString;

              sprintf(tmp, "%d", $1);
              //sprintf(tmp, "%d\0", $1);
              theString = MNewPtr(sizeof(char) * (strlen(tmp) + 1));
              sprintf(theString, "%d", $1);
              //sprintf(theString, "%d\0", $1);
              
              VLAppend(stringList, theString);
           };

FList: FList tFloat { 
              FLAppend(floatList, $2);
           } |
           tFloat { 
              FLAppend(floatList, $1);
           } |
           FList tInteger { 
              FLAppend(floatList, (float)$2);
           } |
           tInteger { 
              FLAppend(floatList, (float)$1);
           };
%%

void _SetProbsFromTable(BeliefNetNode bnn, FloatListPtr fList) {
   BeliefNetNode parent;
   int i, j, incrementDone, value;
   ExamplePtr e = ExampleNew(BNGetExampleSpec(bn));


   for(i = 0 ; i < BNNodeGetNumParents(bnn) ; i++) {
      ExampleSetDiscreteAttributeValue(e, BNNodeGetParentID(bnn, i), 0);
   }
   ExampleSetDiscreteAttributeValue(e, BNNodeGetID(bnn), 0);

   for(i = 0 ; i < FLLength(fList) ; i++) {
      BNNodeSetCP(bnn, e, FLIndex(fList, i));

      incrementDone = 0;
      for(j = BNNodeGetNumParents(bnn) - 1 ; j >= 0 && !incrementDone ; j--) {
         parent = BNNodeGetParent(bnn, j);
         value = ExampleGetDiscreteAttributeValue(e, BNNodeGetID(parent));

         if((value + 1) < BNNodeGetNumValues(parent)) {
            /* +1 in the if corrects for zero indexing in the values */

            ExampleSetDiscreteAttributeValue(e, BNNodeGetParentID(bnn, j),
                                                                   value + 1);
            incrementDone = 1;
         } else {
            /* this attribute is used up, spill over to the next one */
            ExampleSetDiscreteAttributeValue(e, BNNodeGetParentID(bnn, j), 0);
         }
      }

      if(!incrementDone) {
         /* changed all parents, need to change node value */
         value = ExampleGetDiscreteAttributeValue(e, BNNodeGetID(bnn));

         if((value + 1) < BNNodeGetNumValues(bnn)) {
            /* +1 in the if corrects for zero indexing in the values */

            ExampleSetDiscreteAttributeValue(e, BNNodeGetID(bnn),
                                                             value + 1);
         } else {
            /* we already got a value for entry in the CPT */
            ExampleFree(e);
            return;
         }
      }
   }

   /* free all memory and reset lists */
   ExampleFree(e);
}

void BIFSetFile(FILE *file);

int biferror(const char *msg) {
   fprintf(stderr, "%s line %d\n", msg, gLineNumber);
   return 0;
}

//BeliefNet BIFParse(const char *file) {
BeliefNet BIFParse(FILE *input) {
   BIFSetFile(input);

   bn = BNNew();
   //bnn = BNNodeNew();

   floatList = FLNew();
   stringList = VLNew();

   gLineNumber = 0;

   if(bifparse()) {
      /* parse failed! */
      fprintf(stderr, "Error in parsing belief net\n");
   }

   //fclose(input);

   /* free extra bnn, and lists */
   VLFree(stringList);
   FLFree(floatList);

   return bn;
}
