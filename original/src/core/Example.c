#include "vfml.h"

#include "ctype.h"
#include <string.h>

#include <math.h>

/*************************/
/* construction routines */
/*************************/
ExamplePtr ExampleNew(ExampleSpecPtr es) {
   int i;

   ExamplePtr e = MNewPtr(sizeof(Example));
   e->attributes = VALNew();
   e->spec = es;

   for(i = 0 ; i < ExampleSpecGetNumAttributes(e->spec) ; i++) {
      VALAppend(e->attributes, MNewPtr(sizeof(AttributeValue)));
   }

   return e;
}

ExamplePtr ExampleNewInitUnknown(ExampleSpecPtr es) {
  int i;

  ExamplePtr e = MNewPtr(sizeof(Example));
  e->attributes = VALNew();
  e->spec = es;
  
  for (i=0; i<ExampleSpecGetNumAttributes(e->spec) ; i++) {
    VALAppend(e->attributes, 0);
  }
  return e;
}


void ExampleFree(ExamplePtr e) {
   int i;
   AttributeValue *av;

   for(i = VALLength(e->attributes) - 1 ; i >= 0 ; i--) {
      av = VALRemove(e->attributes, i);
      if(av != 0) { MFreePtr(av); }
   }

   VALFree(e->attributes);
   MFreePtr(e);
}

ExamplePtr ExampleClone(ExamplePtr e) {
   ExamplePtr clone = ExampleNew(e->spec);
   int i;

   for(i = 0 ; i < ExampleSpecGetNumAttributes(e->spec) ; i++) {
      if(!ExampleIsAttributeUnknown(e, i)) {
         if(ExampleSpecIsAttributeContinuous(e->spec, i)) {
            ExampleSetContinuousAttributeValue(clone, i,
                ExampleGetContinuousAttributeValue(e, i));
         } else {
            ExampleSetDiscreteAttributeValue(clone, i,
                ExampleGetDiscreteAttributeValue(e, i));
         }
      }
   }

   return clone;
}

void ExampleSetAttributeUnknown(ExamplePtr e, int attNum) {
   AttributeValue *av = VALIndex(e->attributes, attNum);

   VALSet(e->attributes, attNum, 0);
   if(av != 0) {
      MFreePtr(av);
   }
}

static void _ExampleSetAttributeKnown(ExamplePtr e, int attNum) {
   if(ExampleIsAttributeUnknown(e, attNum)) {
      VALSet(e->attributes, attNum, MNewPtr(sizeof(AttributeValue)));
   }
}

void ExampleSetDiscreteAttributeValue(ExamplePtr e, int attNum, int value) {
   _ExampleSetAttributeKnown(e, attNum);
   ((AttributeValue *)VALIndex(e->attributes, attNum))->discreteValue = value;
}

void ExampleSetContinuousAttributeValue(ExamplePtr e,
                                      int attNum, double value) {
   _ExampleSetAttributeKnown(e, attNum);
   ((AttributeValue *)VALIndex(e->attributes, attNum))->continuousValue 
                                                                   = value;
}

void ExampleSetClass(ExamplePtr e, int theClass) {
   e->myclass = theClass;
}

void ExampleSetClassUnknown(ExamplePtr e) {
   e->myclass = -1;
}

void ExampleAddNoise(ExamplePtr e, float p, int doClass, int attrib) {
   int prob = (int)(p * 1000);
   int i;
   int oldValue, newValue;

   if(doClass && (ExampleSpecGetNumClasses(e->spec) > 1)) {
      if(RandomRange(0, 999) < prob) {
         oldValue = ExampleGetClass(e);
         newValue = RandomRange(0, ExampleSpecGetNumClasses(e->spec) - 2);
         if(newValue >= oldValue) {
            newValue++;
         } else {
            /* want to use the same number of calls to random no matter
                 what so generating with the same seed will make the
                 same data */
            RandomRange(0, ExampleSpecGetNumClasses(e->spec) - 2);
         }

         ExampleSetClass(e, newValue);
      }
   }

   if(attrib) {
      for(i = 0 ; i < ExampleSpecGetNumAttributes(e->spec) ; i++) {
         if(ExampleSpecGetAttributeType(e->spec, i) == asDiscreteNamed ||
              ExampleSpecGetAttributeType(e->spec, i) == asDiscreteNoName) {
            if(RandomRange(0, 999) < prob) {
               oldValue = ExampleGetDiscreteAttributeValue(e, i);
               newValue = RandomRange(0, 
                    ExampleSpecGetAttributeValueCount(e->spec, i) - 2);
               if(newValue >= oldValue) {
                  newValue++;
               }
               ExampleSetDiscreteAttributeValue(e, i, newValue);

            } else {
               /* burn a call to random */
               RandomRange(0, 
                    ExampleSpecGetAttributeValueCount(e->spec, i) - 2);
            }
         } else {
            /* this is a continuous attribute */
            ExampleSetContinuousAttributeValue(e, i,
               ExampleGetContinuousAttributeValue(e, i) +
               RandomGaussian(0, p));
	 }
      }
   }
}

//ExamplePtr ParseEIN(FILE *file, ExampleSpecPtr es);

//ExamplePtr ExampleRead(FILE *file, ExampleSpecPtr es) {
//   return ParseEIN(file, es);
//}

static void _SkipWhitespace(FILE *file) {
   char c;

   c = fgetc(file);

   while(isspace(c)) {
      c = fgetc(file);
   }

   ungetc(c, file);
}

int ReadName(FILE *f, char *s); /* from getnames.c */
static void _GetString(FILE *file, char *str) {
   if(!ReadName(file, str)) {
      *str = 0;
   }
}


int _numExamplesRead = 0;

ExamplePtr ExampleRead(FILE *file, ExampleSpecPtr es) {
   ExamplePtr e = ExampleNew(es);
   char c;
   int i;
   int error = 0;
   int index;
   char str[500];
   double continuousValue;

   _SkipWhitespace(file);
   if(feof(file)) {
      ExampleFree(e);
      return 0;
   }

   _numExamplesRead++;
   for(i = 0 ; i < ExampleSpecGetNumAttributes(es) && !error ; i++) {
      _SkipWhitespace(file);

      /* check for unknown */
      c = fgetc(file);
      if(c == '?') {
         _SkipWhitespace(file);
         /* skip the comma */
         c = fgetc(file);
         ExampleSetAttributeUnknown(e, i);
      } else {
         ungetc(c, file);
         /* read something of the appropriate type */
         switch(ExampleSpecGetAttributeType(es, i)) {
          case asIgnore:
             /* read something but ignore it */
            _GetString(file, str);
            ExampleSetAttributeUnknown(e, i);
            break;
          case asContinuous:
            if((i == 0) && feof(file)) {
               error = 1;
               ExampleFree(e);
               e = 0;
            } else {
               _GetString(file, str);

               sscanf(str, "%lf", &continuousValue);
	       //printf("%s %lf\n", str, continuousValue);

               _SkipWhitespace(file);
               c = fgetc(file);
	       //printf("   %c\n", c);
               if(c != ',') {
                  ungetc(c, file);
               }
               ExampleSetContinuousAttributeValue(e, i, continuousValue);
            }
            break;
          case asDiscreteNamed:
          case asDiscreteNoName:
            _GetString(file, str);
            index = ExampleSpecLookupAttributeValueName(es, i, str);
            if(index == -1) { /* it isn't there yet */
               ExampleSpecAddAttributeValue(es, i, str);
               index = ExampleSpecLookupAttributeValueName(es, i, str);
               if(index == -1) { /* we couldn't add it, game over! */
                  if(!(strlen(str) == 0) || !(i == 0)) { /* unless we are done */
                     printf("error reading attribute %d example #%d, Got '%s'\n", i, _numExamplesRead, str);
                  }
                  error = 1;
                  ExampleFree(e);
                  e = 0;
               }
            }
            
            if(!error) {
               ExampleSetDiscreteAttributeValue(e, i, index);
            }
            break;
         }
      }
   }

   if(!error) {
      _GetString(file, str);
      index = ExampleSpecLookupClassName(es, str);
      if(index == -1) {
         printf("error reading class of example #%d, got '%s'\n", _numExamplesRead, str);
	 //ExampleSpecWrite(es, stdout);
         error = 1;
         ExampleFree(e);
         e = 0;
      } else {
         ExampleSetClass(e, index);
      }
   } else {
      printf("error while reading example #%d.\n", _numExamplesRead);
   }

   return e;
}

VoidListPtr ExamplesRead(FILE *file, ExampleSpecPtr es) {
   VoidListPtr outList = VLNew();
   ExamplePtr e = ExampleRead(file, es);

   while(e != 0) {
      VLAppend(outList, e);
      e = ExampleRead(file, es);
   }

   return outList;
}


/*************************/
/* Accessor routines     */
/*************************/
static AttributeType _ExampleGetAttributeType(ExamplePtr e, int attNum) {
   AttributeSpecType type = ExampleSpecGetAttributeType(e->spec, attNum);

   if(type == asContinuous) { return Continuous; }

   return Discrete;
}

int ExampleIsAttributeDiscrete(ExamplePtr e, int attNum) {
   return _ExampleGetAttributeType(e, attNum) == Discrete;
}

int ExampleIsAttributeContinuous(ExamplePtr e, int attNum) {
   return _ExampleGetAttributeType(e, attNum) == Continuous;
}

//int ExampleIsAttributeUnknown(ExamplePtr e, int attNum) {
//   AttributeValue *av = VALIndex(e->attributes, attNum);
//
//   return av == 0;
//}

//int ExampleGetNumAttributes(ExamplePtr e) {
//   return VALLength(e->attributes);
//}

int ExampleGetDiscreteAttributeValue(ExamplePtr e, int attNum) {
   if(ExampleIsAttributeUnknown(e, attNum)) {
      return -1;
   }

   return ((AttributeValue *)VALIndex(e->attributes, attNum))->discreteValue;
}

double ExampleGetContinuousAttributeValue(ExamplePtr e, int attNum) {
   if(ExampleIsAttributeUnknown(e, attNum)) {
      return -1;
   }

   return ((AttributeValue *)VALIndex(e->attributes, attNum))->continuousValue;
}

//int ExampleGetClass(ExamplePtr e) {
//   return e->myclass;
//}

int ExampleIsClassUnknown(ExamplePtr e) {
   return e->myclass == -1;
}

float ExampleDistance(ExamplePtr e, ExamplePtr dst) {
   int i;
   double deltaSquares = 0;
   double thisDelta;

   for(i = 0 ; i < VALLength(e->attributes) ; i++) {
      if(!(ExampleIsAttributeUnknown(e, i)
             || ExampleIsAttributeUnknown(dst, i))) {
         if(ExampleIsAttributeDiscrete(e, i)) {
            /* HERE update with a better discrete distance metric */
            deltaSquares += 0;
         } else {
            thisDelta = ExampleGetContinuousAttributeValue(e, i) -
                          ExampleGetContinuousAttributeValue(dst, i);
            deltaSquares += thisDelta * thisDelta;
         }
      }
   }

   if(deltaSquares > 0) {
      return sqrt(deltaSquares);
   } else {
      return 0;
   }
}

/*************************/
/* Display routines      */
/*************************/
static void _OutputStringEscaped(char *str, FILE *out) {
   while(*str) {
      if(*str == ':' || *str == '?' || *str == ',' || *str == '.') {
          //fprintf(out, "\\");
          fputc('\\', out);
          //fputc('\', out);
      }
      //fprintf(out, "%c", *str++);
      fputc(*str++, out);
   }
}

void ExampleWrite(ExamplePtr e, FILE *out) {
   int i;
   AttributeSpecType type;

   for(i = 0 ; i < VALLength(e->attributes) ; i++) {
      type = ExampleSpecGetAttributeType(e->spec, i);
      if(ExampleIsAttributeUnknown(e, i)) {
         fprintf(out, "?");
      } else if(type == asContinuous) {
         fprintf(out, "%.10g", ExampleGetContinuousAttributeValue(e, i));
      } else if(type == asDiscreteNoName) {
         _OutputStringEscaped(ExampleSpecGetAttributeValueName(e->spec, 
                              i, ExampleGetDiscreteAttributeValue(e, i)),
                             out);
         //fprintf(out, "%s", ExampleSpecGetAttributeValueName(e->spec, 
         //                     i, ExampleGetDiscreteAttributeValue(e, i)));
      } else if(type == asDiscreteNamed) {
         _OutputStringEscaped(ExampleSpecGetAttributeValueName(e->spec, 
                              i, ExampleGetDiscreteAttributeValue(e, i)),
                             out);
         //fprintf(out, "%s", ExampleSpecGetAttributeValueName(e->spec, 
         //                     i, ExampleGetDiscreteAttributeValue(e, i)));
      } else if(type == asIgnore) {
         fprintf(out, "?");
      }

      if(i != VALLength(e->attributes) - 1) {
         fprintf(out, ", ");
      } else {
         if(ExampleIsClassUnknown(e)) {
            fprintf(out, ", ?");
         } else if (ExampleGetClass(e) < ExampleSpecGetNumClasses(e->spec) &&
                              ExampleGetClass(e) >= 0) {
            fprintf(out, ", %s", ExampleSpecGetClassValueName(e->spec,
                               ExampleGetClass(e)));
         } else {
            fprintf(out, "(out of range: %d)", ExampleGetClass(e));
         }
      }
   }

   fprintf(out, "\n");
}
