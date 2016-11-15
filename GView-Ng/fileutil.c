
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

#include "fileutil.h"

int file_exists(const char *path)
{
   return access(path, R_OK) == 0;
}

long file_size(const char *path)
{
  struct stat buf;
  if(stat(path, &buf) == -1)
    return -1;
  return buf.st_size;
}

char *get_file_handle(const char *filename, long *fsz)
{
   int     fd;
   void   *handle;

   if(!file_exists(filename)) {
      fprintf(stderr, "%s does not exist\n", filename);
      exit(1);
   }
   *fsz = file_size(filename);
   fd = open(filename, O_RDONLY);
   assert(fd > 0);
   handle = mmap(NULL, *fsz, PROT_READ, MAP_PRIVATE, fd, 0);
   if(handle == MAP_FAILED) {
      fprintf(stderr, "Error mapping %s to memory", filename);
      exit(1);
   }
   close(fd);
   return (char *)handle;
}

