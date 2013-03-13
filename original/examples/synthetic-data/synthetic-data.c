#include "uwml.h"
#include <stdio.h>

ExampleGeneratorPtr eg;

static ExamplePtr _makeBanana(void) {
   ExamplePtr e;

   e = ExampleGeneratorGenerate(eg);

}

int main(void) {
   ExampleSpecPtr es = ExampleSpecRead("test.names");
   ExamplePtr e;
   int i;

   RandomInit();
   eg = ExampleGeneratorNew(es, -1);

   for(i = 0 ; i < 10 ; i++) {
      e = _makeBanana();

      ExampleWrite(e, stdout);

      /* now move on to the next example */
      ExampleFree(e);
   }

}



