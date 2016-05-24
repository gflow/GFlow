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
#include <fcntl.h>
#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

static int file_exists(const char *path)
{
   return access(path, R_OK) == 0;
}

static long file_size(const char *path)
{
  struct stat buf;
  if(stat(path, &buf) == -1)
    return -1;
  return buf.st_size;
}

static float *parse_amp(const char *fname, int *count)
{
   int fd;
   long fsz;
   char *zbuf;
   z_stream strm = { 0 };
   float *values;
   int err;
printf("Parsing %s ...\n", fname);
   fsz = file_size(fname);
   zbuf = malloc(fsz);
   fd = open(fname, O_RDONLY);
   read(fd, zbuf, fsz);
   close(fd);
//printf("read %zu bytes\n", fsz);
   
   strm.total_in  = strm.avail_in  = fsz;
   strm.total_out = strm.avail_out = sizeof(int);
   strm.next_in   = (Bytef *) zbuf;
   strm.next_out  = (Bytef *) count;

   strm.zalloc = Z_NULL;
   strm.zfree  = Z_NULL;
   strm.opaque = Z_NULL;

   err = inflateInit2(&strm, (15 + 32));
   assert(err == Z_OK);
   err = inflate(&strm, Z_NO_FLUSH);
   assert(err == Z_OK);

   values = (float *)malloc(sizeof(float) * (*count));

   strm.total_out = strm.avail_out = *count * sizeof(float);
   strm.next_out  = (Bytef *) values;
   err = inflate(&strm, Z_FINISH);
   assert(err == Z_STREAM_END);

   free(zbuf);
//printf("done (%d)\n", *count);
   return values;
}

int main(int argc, char *argv[])
{
   gzFile *f;
   float  *result = NULL;
   int     count, i, j;
   char   *output_name = "result.amp";

   printf("argc = %d\n", argc);
   for(i = 1; i < argc; i++) {
      if(strcmp(argv[i], "-o") == 0) {
         output_name = strdup(argv[++i]);
      }
      else if(file_exists(argv[i])) {
         if(result == NULL) {
            result = parse_amp(argv[i], &count);
         }
         else {
            float *amp = parse_amp(argv[i], &count);
            for(j = 0; j < count; j++) {
               result[j] += amp[j];
            }
            free(amp);
         }
      }
      else {
         printf("skipping %s\n", argv[i]);
      }
   }

   f = gzopen(output_name, "w");
   gzwrite(f, &count, sizeof(int));
   gzwrite(f, result, sizeof(float) * count);
   gzclose(f);
   free(result);

   return 0;
}
