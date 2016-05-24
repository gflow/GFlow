/* Copyright (C) 2016, Edward Duffy <eduffy@clemson.edu>

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */


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
