/* Linblow */
#ifndef COMMON_UTIL_H
#define COMMON_UTIL_H

#include <common/types.h>

char * basename(const char *filename);

int build_file_path(char *out, size_t outsz, size_t *pLen, const char *a, const char *b, ...);
int getfilesize(const char *path, int *pSize);
int format_file_size(char * buf, size_t bufsz, int nbytes, int si, int dp);

#endif /* COMMON_UTIL_H */
