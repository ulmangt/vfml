#include "ExampleSpec.h"
#include "../util/memory.h"
#include <string.h>

static void _OutputStringEscaped(char *str, FILE *out) {
   while(*str) {
      if(*str == ':' || *str == '?' || *str == ',' || *str == '.') {
         fprintf(out, "\\");
      }
      fprintf(out, "%c", *str++);
   }
}


AttributeSpecPtr AttributeSpecNew(void) {
   AttributeSpecPtr as = MNewPtr(sizeof(AttributeSpec));

   as->attributeName = 0;

   as->attributeValues = VALNew();
   as->numDiscreteValues = 0;

   return as;
}

void AttributeSpecFree(AttributeSpecPtr as) {
   int i;

   for(i = VALLength(as->attributeValues) - 1 ; i >= 0 ; i--) {
      MFreePtr(VALRemove(as->attributeValues, i));
   }

   VALFree(as->attributeValues);

   if(as->attributeName != 0) {
      MFreePtr(as->attributeName);
   }

   MFreePtr(as);
}

void AttributeSpecSetType(AttributeSpecPtr as, AttributeSpecType type) {
   as->type = type;
}

void AttributeSpecSetName(AttributeSpecPtr as, char *name) {
   as->attributeName = name;
}

void AttributeSpecSetNumValues(AttributeSpecPtr as, int num) {
   as->numDiscreteValues = num;
}

int AttributeSpecGetNumAssignedValues(AttributeSpecPtr as) {
   return VALLength(as->attributeValues);
}

void AttributeSpecAddValue(AttributeSpecPtr as, char *value) {
   if(VALLength(as->attributeValues) < as->numDiscreteValues) {
      VALAppend(as->attributeValues, value);
   } /* HERE else report an error maybe? */
}

/*************************/
/* Accessor routines     */
/*************************/
//AttributeSpecType AttributeSpecGetType(AttributeSpecPtr as) {
//   return as->type;
//}

//int AttributeSpecGetNumValues(AttributeSpecPtr as) {
//   HERE continuous case? 
//   return as->numDiscreteValues;
//}

char *AttributeSpecGetName(AttributeSpecPtr as) {
   return as->attributeName;
}

//char *AttributeSpecGetValueName(AttributeSpecPtr as, int index) {
//   return VALIndex(as->attributeValues, index);
//}

int AttributeSpecLookupName(AttributeSpecPtr as, char *name) {
   int i;

   for(i = 0 ; i < VALLength(as->attributeValues) ; i++) {
      if(strcmp(VALIndex(as->attributeValues, i), name) == 0) {
         return i;
      }
   }

   return -1;
}

/*************************/
/* Display routines      */
/*************************/
void AttributeSpecWrite(AttributeSpecPtr as, FILE *out) {
   int i;

   _OutputStringEscaped(as->attributeName, out);
   fprintf(out, ": ");

   if(as->type == asIgnore) {
      fprintf(out, "ignore.\n");
   } else if(as->type == asContinuous) {
      fprintf(out, "continuous.\n");
   } else if(as->type == asDiscreteNoName) {
      fprintf(out, "discrete %d.\n", as->numDiscreteValues);
   } else {
     //      fprintf(out, "discrete ");
      for(i = 0 ; i < VALLength(as->attributeValues) ; i++) {
         _OutputStringEscaped(VALIndex(as->attributeValues, i), out);
      
         if(i != VALLength(as->attributeValues) - 1) {
            fprintf(out, ", ");
         }
      }
      fprintf(out, ".\n");
   }
}



/*************************/
/* construction routines */
/*************************/
ExampleSpecPtr ExampleSpecNew(void) {
   ExampleSpecPtr es = MNewPtr(sizeof(ExampleSpec));

   es->classes = VALNew();
   es->attributes = VALNew();
   return es;
}

void ExampleSpecFree(ExampleSpecPtr es) {
   int i;

   for(i = VALLength(es->classes) - 1 ; i >= 0 ; i--) {
      MFreePtr(VALRemove(es->classes, i));
   }
   VALFree(es->classes);

   for(i = VALLength(es->attributes) - 1 ; i >= 0 ; i--) {
      AttributeSpecFree(VALRemove(es->attributes, i));
   }
   VALFree(es->attributes);

   MFreePtr(es);
}

void ExampleSpecAddClass(ExampleSpecPtr es, char *className) {
   VALAppend(es->classes, className);
}

void ExampleSpecAddAttributeSpec(ExampleSpecPtr es, AttributeSpecPtr as) {
   VALAppend(es->attributes, as);
}

int ExampleSpecAddDiscreteAttribute(ExampleSpecPtr es, char *name) {
   AttributeSpecPtr as;

   as = AttributeSpecNew();
   AttributeSpecSetName(as, name);
   AttributeSpecSetType(as, asDiscreteNamed);

   VALAppend(es->attributes, as);

   return VALLength(es->attributes) - 1;
}

int ExampleSpecAddContinuousAttribute(ExampleSpecPtr es, char *name) {
   AttributeSpecPtr as;

   as = AttributeSpecNew();
   AttributeSpecSetName(as, name);
   AttributeSpecSetType(as, asContinuous);

   VALAppend(es->attributes, as);

   return VALLength(es->attributes) - 1;
}

void ExampleSpecAddAttributeValue(ExampleSpecPtr es, int attNum, char *val) {
   AttributeSpecSetNumValues(VALIndex(es->attributes, attNum),
           AttributeSpecGetNumValues(VALIndex(es->attributes, attNum)) + 1);
   AttributeSpecAddValue(VALIndex(es->attributes, attNum), val);
}

ExampleSpecPtr ParseFC45(const char *file);

ExampleSpecPtr ExampleSpecRead(char *fileName) {
   return ParseFC45(fileName);
}

void ExampleSpecIgnoreAttribute(ExampleSpecPtr es, int num) {
   AttributeSpecPtr as = VALIndex(es->attributes, num);

   if(as) {
      AttributeSpecFree(as);
   }
   as = AttributeSpecNew();
   AttributeSpecSetType(as, asIgnore);
   VALSet(es->attributes, num, as);
}


/*************************/
/* Accessor routines     */
/*************************/
int ExampleSpecGetNumAttributes(ExampleSpecPtr es) {
   return VALLength(es->attributes);
}

//AttributeSpecType ExampleSpecGetAttributeType(ExampleSpecPtr es, int num) {
//   return AttributeSpecGetType(VALIndex(es->attributes, num));
//}

int ExampleSpecIsAttributeDiscrete(ExampleSpecPtr es, int num) {
   AttributeSpecType ast = 
         ExampleSpecGetAttributeType(es, num);

   return ast == asDiscreteNamed || ast == asDiscreteNoName;
}

int ExampleSpecIsAttributeContinuous(ExampleSpecPtr es, int num) {
   AttributeSpecType ast = 
         ExampleSpecGetAttributeType(es, num);

   return ast == asContinuous;
}

int ExampleSpecIsAttributeIgnored(ExampleSpecPtr es, int num) {
   AttributeSpecType ast = 
         ExampleSpecGetAttributeType(es, num);

   return ast == asIgnore;
}

//int ExampleSpecGetAttributeValueCount(ExampleSpecPtr es, int attNum) {
//   return AttributeSpecGetNumValues(VALIndex(es->attributes, attNum));
//}

int ExampleSpecGetAssignedAttributesValueCount(ExampleSpecPtr es, int attNum) {
   return AttributeSpecGetNumAssignedValues(VALIndex(es->attributes, attNum));
}

char *ExampleSpecGetAttributeName(ExampleSpecPtr es, int attNum) {
   return AttributeSpecGetName(VALIndex(es->attributes, attNum));
}

//char *ExampleSpecGetAttributeValueName(ExampleSpecPtr es,
//                                       int attNum, int valNum) {
//   return AttributeSpecGetValueName(VALIndex(es->attributes, attNum), valNum);
//}

int ExampleSpecLookupAttributeName(ExampleSpecPtr es, char *attributeName) {
   AttributeSpec *as;
   int i, answer;
   
   answer = -1;
   for(i = 0 ; i < VLLength(es->attributes) && answer == -1 ; i++) {
      as = VLIndex(es->attributes, i);
      if(strcmp(attributeName, as->attributeName) == 0) {
         answer = i;
      }
   }

   return answer;
}


int ExampleSpecLookupAttributeValueName(ExampleSpecPtr es, int attNum,
                                   char *valName) {

   return AttributeSpecLookupName(VALIndex(es->attributes, attNum), valName);
}

int ExampleSpecLookupClassName(ExampleSpecPtr es, char *name) {
   int i;

   for(i = 0 ; i < VALLength(es->classes) ; i++) {
      if(strcmp(VALIndex(es->classes, i), name) == 0) {
         return i;
      }
   }

   return -1;
}

char *ExampleSpecGetClassValueName(ExampleSpecPtr es, int classNum) {
   if(classNum < VALLength(es->classes)) {
      return VALIndex(es->classes, classNum);
   } else {
      /* HERE maybe this should return something smarter? */
      return VALIndex(es->classes, 0);
   }
}

int ExampleSpecGetNumClasses(ExampleSpecPtr es) {
   return VALLength(es->classes);
}

/*************************/
/* Display routines      */
/*************************/

void ExampleSpecWrite(ExampleSpecPtr es, FILE *out) {
   int i;

   for(i = 0 ; i < VALLength(es->classes) ; i++) {
      _OutputStringEscaped(VALIndex(es->classes, i), out);
      
      if(i != VALLength(es->classes) - 1) {
         fprintf(out, ", ");
      }
   }
   fprintf(out, ".\n");

   for(i = 0 ; i < VALLength(es->attributes) ; i++) {
      AttributeSpecWrite(VALIndex(es->attributes, i), out);
   }
}


