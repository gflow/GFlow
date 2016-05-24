
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <mpi.h>

#include "util.h"

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

void message(const char *fmt, ...)
{
//   int rank=-1;
   char buff[80];
   time_t rawtime;
   struct tm *tminfo;
   va_list ap;

 //  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   time(&rawtime);
   tminfo = localtime(&rawtime);
   strftime(buff, 80, "%c", tminfo);
   printf("%s >> ", buff);
   // printf("%2d. %s >> ", rank, buff);
   va_start(ap, fmt);
   vprintf(fmt, ap);
   va_end(ap);
}

double microtime()
{
   struct timeval tv;
   gettimeofday(&tv, NULL);
   return (double)tv.tv_sec + tv.tv_usec / 1e6;
}
