#ifndef DEBUGH
#define DEBUGH

#include "sysdefines.h"


/** \ingroup UtilAPI
*/

/** \file

\brief A set of functions that help your programs produce debugging
output in a consistent way.

*/

/** \brief Outputs string to the current target if condition is non-zero. */
void DebugWarn(int condition, char *str);

/** \brief If condition is non-zero outputs string to the current target and halts the program. */
void DebugError(int condidion, char *str);




/** \brief Sets the level of message that will be reported by DebugMessage.

Defaults to 0. 
*/
void DebugSetMessageLevel(int level);

/** \brief Returns the current message level. */
int  DebugGetMessageLevel(void);

/** \brief Formats and prints a message if conditions are met.

If the condition is non-zero and the current message level is greater
than level this calls printf with str as the format string, and any
additional arguments as arguments.
*/
void DebugMessage(int condition, int level, char *str, ...);


/** \brief Sets the target for debugging output.

Defaults to stdout. All future calls to the Debug functions will sent
output to the specified file.
*/
void DebugSetTarget(FILE *target);

/** \brief Gets the current target for debugging output.  */
FILE *DebugGetTarget(void);

#endif
