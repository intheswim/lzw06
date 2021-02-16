#pragma once

#include "export.h"

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#define PACKER_VERSION  0
#define VARIABLE_WIDTH  0
#define MAX_BITS        12

/* compress/decompress flags */

#define true  1
#define false 0

#define BUFFLEN         16384    /* the larger, the better for compression */
#define OUTLEN          3078     /* must be divisible by 3 because of 12-bit per code; does not affect compression. */

enum { HT_SIZE = 8192, HT_KEY_MASK = 8191, HT_CLEAR_CODE = 4094, EOF_CODE = 4095, HT_MAX_CODE = 4096 };

int is_big_endian(void);
void cleanup (const char *outfile, int flags);
int file_exists (const char *filename);
char *str_dup (const char *s);
