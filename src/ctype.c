/* ctype.c */

/**
 * barebones implementation of ctype functions
 * no uppercase letters anywhere because yeh demanded it
 */

 #include "ctype.h"

 /* ascii table basics:
  * control chars: 0x00–0x1f, 0x7f
  * printable: 0x20–0x7e
  * digits: '0'–'9' (0x30–0x39)
  * uppercase: 'A'–'Z' (0x41–0x5a)
  * lowercase: 'a'–'z' (0x61–0x7a)
  * hex digits: digits or 'a'–'f' or 'A'–'F'
  * blank: space (0x20) or tab (0x09)
  * space chars: space, tab, '\n', '\v', '\f', '\r'
  * punctuation: any printable that's not alnum or space
  */
 
 /* --- classification --- */
 
 int isalnum(int c) {
     return (c >= '0' && c <= '9')
         || (c >= 'A' && c <= 'Z')
         || (c >= 'a' && c <= 'z');
 }
 
 int isalpha(int c) {
     return (c >= 'A' && c <= 'Z')
         || (c >= 'a' && c <= 'z');
 }
 
 int iscntrl(int c) {
     return (c >= 0 && c <= 0x1f) || (c == 0x7f);
 }
 
 int isdigit(int c) {
     return (c >= '0' && c <= '9');
 }
 
 int isgraph(int c) {
     return (c >= 0x21 && c <= 0x7e);
 }
 
 int islower(int c) {
     return (c >= 'a' && c <= 'z');
 }
 
 int isprint(int c) {
     return (c >= 0x20 && c <= 0x7e);
 }
 
 int ispunct(int c) {
     if (!isprint(c) || isalnum(c) || c == ' ')
         return 0;
     return 1;
 }
 
 int isspace(int c) {
     return (c == ' ' || c == '\f' || c == '\n'
          || c == '\r' || c == '\t' || c == '\v');
 }
 
 int isupper(int c) {
     return (c >= 'A' && c <= 'Z');
 }
 
 int isxdigit(int c) {
     return (c >= '0' && c <= '9')
         || (c >= 'A' && c <= 'F')
         || (c >= 'a' && c <= 'f');
 }
 
 int isblank(int c) {
     return (c == ' ' || c == '\t');
 }
 
 /* --- conversion --- */
 
 int tolower(int c) {
     if (c >= 'A' && c <= 'Z')
         return c - 'A' + 'a';
     return c;
 }
 
 int toupper(int c) {
     if (c >= 'a' && c <= 'z')
         return c - 'a' + 'A';
     return c;
 }
 