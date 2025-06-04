/* ctype.h */

#ifndef ctype_h
#define ctype_h

#include <stddef.h>

/* --- character classification --- */
int isalnum(int c);
int isalpha(int c);
int iscntrl(int c);
int isdigit(int c);
int isgraph(int c);
int islower(int c);
int isprint(int c);
int ispunct(int c);
int isspace(int c);
int isupper(int c);
int isxdigit(int c);
int isblank(int c);

/* --- character conversion --- */
int tolower(int c);
int toupper(int c);

#endif
