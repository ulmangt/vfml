#ifndef SYSDEFINESH
#define SYSDEFINESH

/* include the needed system headers */
#if defined(WIN32)
   #include <stdlib.h>
   #include <stdio.h>
   #include <string.h>
   #include <stdarg.h>
#elif defined(UNIX) | defined(CYGNUS)
   #include <stdlib.h>
   #include <stdio.h>
   #include <string.h>
   #include <stdarg.h>
   #include <unistd.h>
#elif defined(PILOT)
   /*#define PILOT_PRECOMPILED_HEADERS_OFF*/
   #ifndef __PILOT_H__ 
      #include <Pilot.h>
   #endif /* __PILOT_H__ */

#endif /* UNIX | PILOT */


#endif /* DEFINESH */
