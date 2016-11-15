
#ifndef FILEUTIL_H
#define FILEUTIL_H

#include <sys/mman.h>

int file_exists(const char *path);
long file_size(const char *path);
char *get_file_handle(const char *filename, long *fsz);

#endif /* FILEUTIL_H */
