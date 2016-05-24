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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <linux/limits.h>

#include <assert.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <float.h>
#include <math.h>
#include <zlib.h>

#define streq(X,Y) (strcmp((X),(Y))==0)

struct GridPoint
{
   double value;
   int visited;
};

struct StackData
{
   int x, y, direction;
};

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

static char *get_file_handle(const char *filename, long *fsz)
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

struct GridPoint **create_grid(int nrows, int ncols)
{
   int i;
   struct GridPoint *data = malloc(sizeof(struct GridPoint) * nrows * ncols);
   struct GridPoint **result = malloc(sizeof(struct GridPoint *) * nrows);
   for(i = 0; i < nrows; i++) {
      result[i] = &data[i * ncols];
   }
   return result;
}

static char *parse_header(char    *grid,
                          int     *ncols,     int    *nrows,
                          double  *xllcorner, double *yllcorner,
                          double  *cellsize,  double *NODATA_value)
{
   char key[32];
   int  bytes;

   while(isalpha(grid[0])) {
       sscanf(grid, "%s%n", key, &bytes);
       grid += bytes;
   
       if(streq(key,"ncols"))
         sscanf(grid, "%d%n", ncols, &bytes);
       else if(streq(key,"nrows"))
         sscanf(grid, "%d%n", nrows, &bytes);
       else if(streq(key,"xllcorner"))
         sscanf(grid, "%lf%n", xllcorner, &bytes);
       else if(streq(key,"yllcorner"))
         sscanf(grid, "%lf%n", yllcorner, &bytes);
       else if(streq(key,"cellsize"))
         sscanf(grid, "%lf%n", cellsize, &bytes);
       else if(streq(key,"NODATA_value"))
         sscanf(grid, "%lf%n", NODATA_value, &bytes);
       grid += bytes;

       while(isspace(grid[0]))
          ++grid;
   }
   return grid;
}

struct GridPoint **parse_grid(char *gfile, int *nrows, int *ncols, int *count,
                              int *x1, int *y1, int *x2, int *y2)
{
   double  xllcorner, yllcorner;
   double  cellsize,  NODATA_value;
   char   *p, *q;
   int     i, j;
   struct GridPoint  **grid;

   long fsz;
   char *rh = get_file_handle(gfile, &fsz);
   p = parse_header(rh,
                    ncols, nrows,
                    &xllcorner, &yllcorner,
                    &cellsize,  &NODATA_value);
   grid = create_grid(*nrows, *ncols);

   *count = 0;
   *x1 = *y1 = *x2 = *y2 = 0;
   for(i = 0; i < *nrows; i++) {
      for(j = 0; j < *ncols; j++) {
         grid[i][j].value   = strtod(p, &q);
         if(grid[i][j].value > 1e-5) {
            ++(*count);
            grid[i][j].visited = 0;
            if(grid[i][j].value >= 0.99) {
               printf("(%d,%d) = %f\n", i, j, grid[i][j].value);
               if(*x1 == 0) { *x1 = i; *y1 = j; }
               else { *x2 = i; *y2 = j; }
            }
         }
         else {
            grid[i][j].visited = 1;
         }
         p = q;
      }
   }
   munmap(rh, fsz);

   return grid;
}

int main(int argc, char *argv[])
{

   int nrows, ncols, count;
   int x1,y1,x2,y2;
   struct GridPoint **grid = parse_grid(argv[1], &nrows, &ncols, &count, &x1, &y1, &x2, &y2);
   printf("nrows = %d, ncols = %d, count = %d\n", nrows, ncols, count);
   struct StackData stack[count];
   double sum = 0;

   stack[0] = (struct StackData){ x1, y1, 0 };
   int p = 0;
   while(p >= 0) {
      int x = stack[p].x, y = stack[p].y;
      if(x < 0 || x >= nrows) { --p; continue; }
      if(y < 0 || y >= ncols) { --p; continue; }
      if(stack[p].direction == 0 && grid[x][y].visited == 1) { --p; continue; }

      if(stack[p].direction == 0)
         sum += grid[x][y].value;
     // if(p%1000 == 0)
     // printf("TOP: p = %d, x = %d, y = %d, direction = %d, value = %lf, sum = %lf\n", p, x, y, stack[p].direction, grid[x][y].value, sum);
      if(x == x2 && y == y2)
      {
         printf("Found target.  Path length = %d.  Current = %lf\n", p, sum);
         --p;
      }
      else if(stack[p].direction < 4)
      {
         grid[x][y].visited = 1;
         ++stack[p].direction;
         switch(stack[p].direction) {
            case 1: stack[++p] = (struct StackData){ x-1, y,   0 };
                    break;

            case 2: stack[++p] = (struct StackData){ x,   y+1, 0 };
                    break;

            case 3: stack[++p] = (struct StackData){ x+1, y,   0 };
                    break;

            case 4: stack[++p] = (struct StackData){ x,   y-1, 0 };
                    break;

            case 5: stack[++p] = (struct StackData){ x-1, y+1, 0 };
                    break;

            case 6: stack[++p] = (struct StackData){ x+1, y+1, 0 };
                    break;

            case 7: stack[++p] = (struct StackData){ x+1, y-1, 0 };
                    break;

            case 8: stack[++p] = (struct StackData){ x-1, y-1, 0 };
                    break;
         }
      }
      else {
         grid[x][y].visited = 0;
         sum -= grid[x][y].value;
        --p;
      }
   }
   printf("DONE.  Sum = %lf\n", sum);

   
}
