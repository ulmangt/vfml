#include "uwml.h"
#include <stdio.h>

int main(void) {
   ExampleSpecPtr es = ExampleSpecRead("test.names");
   ExamplePtr e;
   FILE *exampleIn = fopen("test.data", "r");
   DecisionTreePtr dt;
   VoidListPtr examples = VLNew();

   e = ExampleRead(exampleIn, es);
   while(e != 0) { /* ExampleRead returns 0 when EOF */
      VLAppend(examples, e);

      /* now move on to the next example */
      e = ExampleRead(exampleIn, es);
   }

   dt = C45Learn(es, examples);

   DecisionTreePrint(dt, stdout);
}


