#include "Debug.h"

int  _DebugLevel = 0;

#if defined(PILOT)
void DebugWarn(int condition, char *str) {
   ErrNonFatalDisplayIf(condition, str);
}

void DebugError(int condition, char *str) {
   ErrFatalDisplayIf(condition, str);
}
#elif defined(UNIX) | defined(CYGNUS) | defined(WIN32)


FILE *_DebugTarget = 0;

static void _DebugInitIfNeeded(void) {
   if(_DebugTarget == 0) {
//      _DebugTarget = stderr;
      _DebugTarget = stdout;
   }
}

void DebugWarn(int condition, char *str) {
   if(condition) {
      _DebugInitIfNeeded();
      fprintf(_DebugTarget, "Warning: %s\n", str);
   }
}

void DebugError(int condition, char *str) {
   if(condition) {
      _DebugInitIfNeeded();
      fprintf(_DebugTarget, "Error: %s\n", str);
      exit(0);
   }
}

void DebugSetMessageLevel(int level) {
   _DebugLevel = level;
}

int  DebugGetMessageLevel(void) {
   return _DebugLevel;
}

void DebugMessage(int condition, int level, char *str, ...) {
   va_list vargs;

   if(condition && (level <= _DebugLevel)) {
      _DebugInitIfNeeded();
      va_start(vargs, str);
      vfprintf(_DebugTarget, str, vargs);
      va_end(vargs);
   }
}

void DebugSetTarget(FILE *target) {
   _DebugTarget = target;
}

FILE *DebugGetTarget(void) {
   return _DebugTarget;
}

#endif /* PILOT | UNIX */
