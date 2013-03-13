%{
#include "fc45.tab.h"
#include "vfml.h"
#include <string.h>
#include <stdlib.h>

  /* HERE  doesn't match strings starting with numbers other than 0 right */

char string_buf[4000]; /* BUG - maybe check for strings that are too long? */
char *string_buf_ptr;

void FC45FinishString(void);

extern int gLineNumber;
%}

%x str_rule

%%
<str_rule,INITIAL>\|[^\n]* ;

[\ \t\r]+ ;
\n      gLineNumber++;

\. { return '.';}
, { return ',';}
: { return ':';}

ignore { return tIgnore; }
continuous { return tContinuous; }
discrete { return tDiscrete; }


[^:?,\t\n\r\|\.\\\ ] string_buf_ptr = string_buf; unput(yytext[0]); BEGIN(str_rule);

<str_rule>[:,?]             FC45FinishString(); unput(yytext[0]); return tString;
<str_rule>\.[\t\r\ ]    FC45FinishString(); unput(yytext[1]); unput(yytext[0]); return tString;
<str_rule>\.\n    FC45FinishString(); unput(yytext[1]); unput(yytext[0]); return tString; gLineNumber++;

<str_rule><<EOF>> {
   int len = strlen(string_buf);
   //   printf("eof rule.\n");
   if(len == 1 && string_buf[0] == '.') {
     //printf("   period at end of file\n");
      return '.';
   } else if(string_buf[len - 1] == '.') {
     // printf("   period: %s - unput .\n", string_buf);

      FC45FinishString(); unput('.'); return tString;
   } else {
     // printf("   no-period: %s\n", string_buf);
      FC45FinishString(); return tString;
   }
}

<str_rule>\\:   *string_buf_ptr++ = ':';
<str_rule>\\\?  *string_buf_ptr++ = '?';
<str_rule>\\,   *string_buf_ptr++ = ',';
<str_rule>\\.    *string_buf_ptr++ = '.';

<str_rule>\n  *string_buf_ptr++ = ' '; gLineNumber++;
<str_rule>[ \t\r]+  *string_buf_ptr++ = ' ';

<str_rule>[^:?,\t\n\r\|\.\\\ ]+        {
   char *yptr = yytext;

   while(*yptr) {
      *string_buf_ptr++ = *yptr++;
   }
}


%%
int fc45wrap(void) {
	return 1;
}

void FC45SetFile(FILE *file) {
	fc45in = file;
	yyrestart(fc45in);
}

void FC45FinishString(void) {
   int len;
   char *tmpStr;

   BEGIN(INITIAL);
   *string_buf_ptr = '\0';

   len = strlen(string_buf);

   /* remove any ending spaces */
   while(string_buf[len - 1] == ' ') {
     string_buf[len - 1] = '\0';
     len--;
   }

   tmpStr = MNewPtr(len + 1);
   strncpy(tmpStr, string_buf, len + 1);
   fc45lval.string = tmpStr;
   string_buf[0] = '\0';
}
