#include <string.h>

char * basename (const char *filename)
{
  char *p;
  char *p1 = strrchr (filename, '/');
  char *p2 = strrchr (filename, '\\');
  p = (uintptr_t)p1 < (uintptr_t)p2 ? p2 : p1;
  return p ? p + 1 : (char *) filename;
}
