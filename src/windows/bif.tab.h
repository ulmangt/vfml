#ifndef BISON_CORE_BIF_TAB_H
# define BISON_CORE_BIF_TAB_H

#ifndef YYSTYPE
typedef union {
   int integer;
   float f;
   char *string;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
# define	tInteger	257
# define	tFloat	258
# define	tString	259
# define	tNetwork	260
# define	tVariable	261
# define	tProbability	262
# define	tProperty	263
# define	tVariableType	264
# define	tDiscrete	265
# define	tDefaultValue	266
# define	tTable	267
# define	tEOF	268


extern YYSTYPE biflval;

#endif /* not BISON_CORE_BIF_TAB_H */
