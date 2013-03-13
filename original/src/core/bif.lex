%{
#include "bif.tab.h"
#include "vfml.h"
#include <string.h>
#include <stdlib.h>

  /* HERE  doesn't match strings starting with numbers other than 0 right */

char string_buf[4000]; /* BUG - maybe check for strings that are too long? */
char *string_buf_ptr;

void BIFFinishString(void);

extern int gLineNumber;
int biferror(const char *msg);
%}


DIGIT           [0-9]+
EXPONANT        [eE][+-]?{DIGIT}+
WORD            [a-zA-Z0-9_\-%][a-zA-Z0-9_\-%]*

%%
[\ \t\r,\|]+ ;

"/*" {
  int c;

  for ( ; ; )
    {
      while ( (c = input()) != '*' &&
	      c != EOF )
	;    /* eat up text of comment */

      if ( c == '*' )
	{
	  while ( (c = input()) == '*' )
	    ;
	  if ( c == '/' )
	    break;    /* found the end */
	}

      if ( c == EOF )
	{
	  biferror( "EOF in comment" );
	  break;
	}
    }
}


"//"[^\n]*(\n|\r\n) {gLineNumber++;}

\n      gLineNumber++;

"(" { return '(';}
")" { return ')';}
"{" { return '{';}
"}" { return '}';}
\[ { return '[';}
\] { return ']';}
; { return ';';}

network { return tNetwork; }
variable { return tVariable; }
probability { return tProbability; }
property" "[^\n]*(\n|\r\n) {
   gLineNumber++;
   return tProperty;
}
type {return tVariableType; }
discrete {return tDiscrete; }
default {return tDefaultValue; }
table {return tTable; }

{DIGIT}+ {
  biflval.integer = atoi(yytext);
  return tInteger;
}

{DIGIT}+"."{DIGIT}*{EXPONANT}?   |
"."{DIGIT}+{EXPONANT}?            |
{DIGIT}+{EXPONANT}? {
  biflval.f = atof(yytext);
  return tFloat;
}

{WORD} {
   int len;
   char *tmpStr;

   len = strlen(yytext);
   tmpStr = (char *)MNewPtr(sizeof(char) * (len + 1));
   strncpy(tmpStr, yytext, len + 1);
   biflval.string = tmpStr;

   return tString;
}

%%
int bifwrap(void) {
	return 1;
}

void BIFSetFile(FILE *file) {
	bifin = file;
	yyrestart(bifin);
}

