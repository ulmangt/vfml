%{
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "vfml.h"
%}

%{
int fc45lex(void);
int fc45error(const char *);

/* HERE figure out how to give better error messages */
/* BUG needs a \n at the end of the names file */

/* These tmps are allocated at the begining of parsing and then
    used during parsing.  For example, so that we can simply
    add terrains to an area while parsing.  After parsing a
    statement, the associated tmp is added to the appropriate
    global list, and a new tmp is allocated.  Finally, at the
    end of parsing, all the tmps are freed
*/

ExampleSpecPtr        exampleSpec;
AttributeSpecPtr      attributeSpec;
int gLineNumber;

%}

%union {
   int integer;
   float f;
   char *string;
}

%token <integer> tInteger
%token <string>  tString

%token tIgnore tContinuous tDiscrete tEOF

%%

ExampleSpec: ClassList '.' AttributeList;

ClassList: ClassList ',' ClassSpec | ClassSpec /* ending */;

ClassSpec: tString { ExampleSpecAddClass(exampleSpec, $1); };

AttributeList: AttributeList AttributeSpec | /* ending */;

AttributeSpec: tString ':' AttributeInfo '.' { 
   AttributeSpecSetName(attributeSpec, $1);
   ExampleSpecAddAttributeSpec(exampleSpec, attributeSpec);
   attributeSpec = AttributeSpecNew();
};

AttributeInfo: tIgnore {
                   AttributeSpecSetType(attributeSpec, asIgnore);} |
               tContinuous {
                   AttributeSpecSetType(attributeSpec, asContinuous);} |
               tDiscrete tString {
                   AttributeSpecSetType(attributeSpec, asDiscreteNoName);
                   AttributeSpecSetNumValues(attributeSpec, atoi($2)); } |
               AttributeValueNameList { 
		 AttributeSpecSetType(attributeSpec, asDiscreteNamed); };

AttributeValueNameList: AttributeValueNameList ',' tString {
   AttributeSpecSetNumValues(attributeSpec,
                     AttributeSpecGetNumValues(attributeSpec) + 1);
   AttributeSpecAddValue(attributeSpec, $3); } |
tString {
   AttributeSpecSetNumValues(attributeSpec,
                     AttributeSpecGetNumValues(attributeSpec) + 1);
   AttributeSpecAddValue(attributeSpec, $1); };

%%

void FC45SetFile(FILE *file);

int fc45error(const char *msg) {
   fprintf(stderr, "%s line %d\n", msg, gLineNumber);
   return 0;
}

ExampleSpecPtr ParseFC45(const char *file) {
   FILE *input;

   input = fopen(file, "r");
   if(input == 0) {
      return 0;
   }

   FC45SetFile(input);

   exampleSpec = ExampleSpecNew();
   attributeSpec = AttributeSpecNew();

   gLineNumber = 0;

   if(fc45parse()) {
      /* parse failed! */
      fprintf(stderr, "Error in parsing: %s\n", file);
   }

   fclose(input);

   /* free the left over attribute spec */
   AttributeSpecFree(attributeSpec);

   return exampleSpec;
}
