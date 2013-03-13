#include "C45interface.h"
#include <stdio.h>
#include <stdlib.h>

#define kTempStem    "_C45interTmp_"

DecisionTreePtr C45Learn(ExampleSpecPtr es, VoidListPtr examples) {
   FILE *names, *data, *tree;
   DecisionTreePtr dt;
   char str[256];
   int i;
   
   /* write the temporary files */
   sprintf(str, "%s.names", kTempStem);
   names = fopen(str, "w");
   ExampleSpecWrite(es, names);
   fclose(names);

   sprintf(str, "%s.data", kTempStem);
   data = fopen(str, "w");
   for(i = 0 ; i < VLLength(examples) ; i++) {
      ExampleWrite(VLIndex(examples,i), data);
   }
   fclose(data);
   
   /* execute c4.5 */
   sprintf(str, "c4.5 -f %s >> /dev/null", kTempStem);
   system(str);

   /* grab the tree */
   sprintf(str, "%s.tree", kTempStem);
   tree = fopen(str, "r");
   dt = DecisionTreeReadC45(tree, es);
   fclose(tree);

   /* clean up */
   sprintf(str, "rm -f %s.names %s.data %s.unpruned %s.tree", kTempStem,
              kTempStem, kTempStem, kTempStem);
   system(str);

   return dt;
}

