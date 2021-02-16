#include "common.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#if defined(__linux__)
    /* Linux  */
#include <unistd.h> /* access */
#elif defined (_WIN32)
#include <io.h>
#endif

int is_big_endian(void)
{
    union {
        uint32_t i;
        char c[4];
    } bint = {0x01020304};

    return (bint.c[0] == 1) ? 1 : 0; 
}

/*--------------------------------------------------------------------*/

void cleanup (const char *outfile, int flags)
{
  if (0 == (flags & KEEP_ON_ERROR))
  {
    remove (outfile);
  }
}

/*--------------------------------------------------------------------*/

int file_exists (const char *filename)
{
#if defined(__linux__)
    return (access(filename, F_OK) == 0) ? 1 : 0; 
#elif defined(_WIN32)
  return (_access(filename, 0) == 0) ? 1 : 0;
#endif
}

/*--------------------------------------------------------------------*/

char *str_dup (const char *s) /* strdup replacement. */
{
  size_t size = strlen (s) + 1;
  char *ret = malloc (size);

  if (!ret) return NULL;
  
  strcpy (ret, s);
  return ret;
}

