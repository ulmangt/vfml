#ifndef BISON_CORE_FC45_TAB_H
# define BISON_CORE_FC45_TAB_H

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
# define	tString	258
# define	tIgnore	259
# define	tContinuous	260
# define	tDiscrete	261
# define	tEOF	262


extern YYSTYPE fc45lval;

#endif /* not BISON_CORE_FC45_TAB_H */
