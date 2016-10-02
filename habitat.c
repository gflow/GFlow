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


#include <sys/mman.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <petsc.h>

#include "habitat.h"
#include "util.h"

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
   close(fd);  /* man page says this is safe */
   return (char *)handle;
}

static void allocate_cells(struct ResistanceGrid *R)
{
   int i;
   struct RCell *data;
   PetscMalloc(sizeof(struct RCell) * R->nrows * R->ncols, &data);
   PetscMalloc(sizeof(struct RCell *) * R->nrows, &R->cells);
   for(i = 0; i < R->nrows; i++) {
      R->cells[i] = &data[i * R->ncols];
   }
}

static char *parse_header(struct ResistanceGrid *R, char *grid)
{
   char key[32];
   int  bytes;

   while(isalpha(grid[0])) {
       sscanf(grid, "%s%n", key, &bytes);
       grid += bytes;
   
       if(streq(key,"ncols"))
         sscanf(grid, "%d%n", &R->ncols, &bytes);
       else if(streq(key,"nrows"))
         sscanf(grid, "%d%n", &R->nrows, &bytes);
       else if(streq(key,"xllcorner"))
         sscanf(grid, "%lf%n", &R->xllcorner, &bytes);
       else if(streq(key,"yllcorner"))
         sscanf(grid, "%lf%n", &R->yllcorner, &bytes);
       else if(streq(key,"cellsize"))
         sscanf(grid, "%lf%n", &R->cellsize, &bytes);
       else if(streq(key,"NODATA_value"))
         sscanf(grid, "%lf%n", &R->NODATA_value, &bytes);
       grid += bytes;

       while(isspace(grid[0]))
          ++grid;
   }
   message("(rows,cols) = (%d,%d)\n", R->nrows, R->ncols);
   return grid;
}

void parse_habitat_file(struct ResistanceGrid *R, const char *habitat_file)
{
   long   fsz;
   char  *habitat;
   char  *p, *q;
   int    i, j;

   habitat = get_file_handle(habitat_file, &fsz);
   p = parse_header(R, habitat);
   allocate_cells(R);

   R->cell_count = 0;
   for(i = 0; i < R->nrows; i++) {
      for(j = 0; j < R->ncols; j++) {
         R->cells[i][j].value = strtod(p, &q);
         // if(R->cells[i][j].value != R->NODATA_value && R->cells[i][j].value != 0.) {
         if(R->cells[i][j].value > 0) {
            R->cells[i][j].index = R->cell_count++;
         }
         else {
            R->cells[i][j].index = -1;
         }
         p = q;
      }
   }
   munmap(habitat, fsz);
}

void free_habitat(struct ResistanceGrid *R)
{
   PetscFree(R->cells[0]);
   PetscFree(R->cells);
}

void discard_islands(struct ResistanceGrid *R)
{
   int    i, j;
   int **labels, *label_buffer;
   typedef struct { int x,y; } ToDo_t;
   ToDo_t *todo;
   int current_label, current_area, top, largest_label, largest_area;

   PetscMalloc(sizeof(int)    * R->nrows * R->ncols, &label_buffer);
   PetscMalloc(sizeof(int *)  * R->nrows, &labels);
   PetscMalloc(sizeof(ToDo_t) * R->nrows * R->ncols, &todo);  /* overkill. will never need to be this large, but no point in making a dynamic allocation scheme for this */
   for(i = 0; i < R->nrows; i++) {
      labels[i] = &label_buffer[i * R->ncols];
   }
   memset(label_buffer, 0, sizeof(int) * R->nrows * R->ncols); /* 0 is the "unseen" label */

   current_label = 1;
   largest_label = 0;
   largest_area  = 0;

   for(i = 0; i < R->nrows; i++) {
      for(j = 0; j < R->ncols; j++) {
         if(labels[i][j] != 0)
            continue;
         if(R->cells[i][j].value <= 0) {  /* NODATA */
            labels[i][j] = -1;
            continue;
         }

	 /* Starting a new contiguous landmass */
         todo[0].x = i; todo[0].y = j; top = 1;
         current_area = 0;
         while(top > 0) {
            ++current_area;
            --top;
            int a = todo[top].x, b = todo[top].y;
            labels[a][b] = current_label;
            /* How long can we get away without checking for diagonal neighbors? */
            if(a > 1 && labels[a-1][b] == 0 && R->cells[a-1][b].value > 0) {
               todo[top].x = a - 1;
               todo[top].y = b;
               ++top;
            }
            if(b > 1 && labels[a][b-1] == 0 && R->cells[a][b-1].value > 0) {
               todo[top].x = a;
               todo[top].y = b - 1;
               ++top;
            }
            if(a < R->nrows-1 && labels[a+1][b] == 0 && R->cells[a+1][b].value > 0) {
               todo[top].x = a + 1;
               todo[top].y = b;
               ++top;
            }
            if(b < R->ncols-1 && labels[a][b+1] == 0 && R->cells[a][b+1].value > 0) {
               todo[top].x = a;
               todo[top].y = b + 1;
               ++top;
            }
         }
         if(current_area > largest_area) {
            largest_label = current_label;
            largest_area  = current_area;
         }
         ++current_label;
      }
   }
   R->cell_count = 0;
   int nremoved = 0;
   for(i = 0; i < R->nrows; i++) {
      for(j = 0; j < R->ncols; j++) {
         if(labels[i][j] == -1)
            continue;
         if(labels[i][j] != largest_label) {
            R->cells[i][j].index = -1;
            R->cells[i][j].value = R->NODATA_value;
            ++nremoved;
         }
         else
            R->cells[i][j].index = R->cell_count++;
      }
   }
   message("Removed %d islands (%d cells).\n", current_label-2, nremoved);

   PetscFree(label_buffer);
   PetscFree(labels);
}

