#pragma once

enum { KEEP_ON_ERROR = 1, VERBOSE_OUTPUT = 2, OVERWRITE_FLAG = 4 };

#ifdef __cplusplus
extern "C"
{
#endif

/* The following functions return 1 on success, 0 on error */
extern int Decompress (const char *, const char *, int flags);
extern int Compress (const char *, const char *, int flags);

#ifdef __cplusplus
} // extern "C"
#endif
