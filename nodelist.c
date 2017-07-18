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


#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <mpi.h>
#include <petsc.h>

#include "util.h"
#include "nodelist.h"
#include "habitat.h"

char       node_file[PATH_MAX] = { 0 };
char       node_pair_file[PATH_MAX] = { 0 };
PetscBool  nearest_first = PETSC_FALSE;
PetscBool  furthest_first = PETSC_FALSE;
PetscInt  shuffle_node_pairs = -1;
PetscReal  max_distance = 40e6;  /* circumference of the earth (approx) */
PetscBool  resistance_only = PETSC_FALSE;

static struct Point *parse_node_list(char *filename, size_t *npoints);
static int validate_points(struct Point *points, size_t npoints, struct ResistanceGrid *R);
static struct PointPairs *generate_pairs(struct Point *points, size_t npoints, double max_pixel_distance);
static struct PointPairs *parse_node_pair_file(struct Point *points, size_t npoints, double max_pixel_distance);
static void sort_pairs_close(struct PointPairs *pairs);
static void sort_pairs_far(struct PointPairs *pairs);
static void shuffle_pairs(struct PointPairs *pairs);

struct PointPairs *init_point_pairs(struct ResistanceGrid *R)
{
   struct Point *points;
   size_t        npoints;
   struct PointPairs *pp;

   points = parse_node_list(node_file, &npoints);
   if(!validate_points(points, npoints, R)) {
      fprintf(stderr, "Input file `%s` contains invalid points.\n", node_file);
//      MPI_Abort(MPI_COMM_WORLD, 1);
   }

   if(strlen(node_pair_file) == 0) {
      pp = generate_pairs(points, npoints, max_distance / R->cellsize);
   }
   else {
      pp = parse_node_pair_file(points, npoints, max_distance / R->cellsize);
   }
   pp->ncount = npoints;
   free(points);

   if(nearest_first)
      sort_pairs_close(pp);
   else if(furthest_first)
      sort_pairs_far(pp);
   else if(shuffle_node_pairs)
      shuffle_pairs(pp);

   return pp;
}

int validate_points(struct Point *points, size_t npoints, struct ResistanceGrid *R)
{
   size_t i;
   int result = 1;
   for(i = 0; i < npoints; i++) {
      if(points[i].x <= 0) {
         fprintf(stderr, "Point #%zu (%ld,%ld) is out of range. (%ld <= 0)\n",
                         i+1, points[i].x, points[i].y, points[i].x);
         result = 0;
      }
      if(points[i].y <= 0) {
         fprintf(stderr, "Point #%zu (%ld,%ld) is out of range. (%ld <= 0)\n",
                         i+1, points[i].x, points[i].y, points[i].y);
         result = 0;
      }
      if(points[i].x >= R->nrows) {
         fprintf(stderr, "Point #%zu (%ld,%ld) is out of range. (%ld >= %d)\n",
                         i+1, points[i].x, points[i].y, points[i].x, R->nrows);
         result = 0;
      }
      if(points[i].y >= R->ncols) {
         fprintf(stderr, "Point #%zu (%ld,%ld) is out of range. (%ld >= %d)\n",
                         i+1, points[i].x, points[i].y, points[i].y, R->ncols);
         result = 0;
      }
      if(R->cells[points[i].x][points[i].y].index == -1) {
         fprintf(stderr, "Point #%zu (%ld,%ld) is invalid.\n", i+1, points[i].x, points[i].y);
         result = 0;
      }
   }
   return result;
}

int peek(FILE *f)
{
   int c = fgetc(f);
   ungetc(c, f);
   return c;
}

static struct Point *node_list_is_pairs(FILE *f, size_t *npoints)
{
   struct Point *points = NULL;
   size_t nmax = 32;

   points = (struct Point *)malloc(nmax * sizeof(struct Point));
   while(!feof(f)) {

      ++(*npoints);
      if(*npoints == nmax) {
         nmax *= 2;
         points = (struct Point *)realloc(points, nmax * sizeof(struct Point));
      }
      points[*npoints].index = *npoints;
      fscanf(f, "%ld", &points[*npoints].x);
      fscanf(f, "%ld", &points[*npoints].y);
      --points[*npoints].x;
      --points[*npoints].y;
   }
   return points;
}

static struct Point *node_list_is_asc(char *filename, size_t *npoints)
{
   struct ResistanceGrid R;
   struct Point *points = NULL;
   size_t nmax = 32;
   unsigned i, j;

   /* We should generalize the ASCII Grid reader */
   parse_habitat_file(&R, filename);
   points = (struct Point *)malloc(nmax * sizeof(struct Point));
   for(i = 0; i < R.nrows; i++) {
      for(j = 0; j < R.ncols; j++) {
         int k = R.cells[i][j].index;
         if(k > -1) {
            if(k >= nmax) {
               nmax *= 2;
               points = (struct Point *)realloc(points, nmax * sizeof(struct Point));
            }
            points[k].index = k;
            points[k].x = i;
            points[k].y = j;
         }
      }
   }
   *npoints = R.cell_count;

   free_habitat(&R);
   return points;
}

struct Point *parse_node_list(char *filename, size_t *npoints)
{
   struct Point *points = NULL;
   FILE *f = fopen(filename, "r");

   *npoints = -1;

   /* This is a terrible approach if we ever want to support multiple formats.  Until
    * then, if the first character is a letter then assume it's an ASCII Grid file
    */
   if(isalpha(peek(f))) {
      points = node_list_is_asc(filename, npoints);
   }
   else
      points = node_list_is_pairs(f, npoints);
   fclose(f);
   message("%zu points in %s\n", *npoints, filename);

   return points;
}

double dist(struct Point p1, struct Point p2)
{
   return hypot(0. + p1.x - p2.x, 0. + p1.y - p2.y);
}

struct PointPairs *generate_pairs(struct Point *points, size_t npoints, double max_pixel_distance)
{
   struct PointPairs *pp = NULL;
   // PetscMalloc(sizeof(struct PointPairs), &pp);
   pp = (struct PointPairs *)malloc(sizeof(struct PointPairs));

   size_t i, j;
   size_t nmax = 32, skip_count = 0;

   message("Max distance: %7.2f pixels\n", max_pixel_distance);
   pp->pairs = (struct Pair *)realloc(NULL, nmax * sizeof(struct Pair));
   pp->count = 0;
   for(i = 0; i < npoints; i++) {
      for(j = i+1; j < npoints; j++) {
         if(dist(points[i], points[j]) <= max_pixel_distance) {
            if(pp->count == nmax) {
               nmax *= 2;
               pp->pairs = (struct Pair *)realloc(pp->pairs, nmax * sizeof(struct Pair));
            }
            pp->pairs[pp->count].p1 = points[i];
            pp->pairs[pp->count].p2 = points[j];
            ++pp->count;
         }
         else {
            ++skip_count;
         }
      }
      if(resistance_only)
        break;
   }
   message("%zu pairs generated.  %zu skipped.\n", pp->count, skip_count);
   return pp;
}

struct PointPairs *parse_node_pair_file(struct Point *points, size_t npoints, double max_pixel_distance)
{
   FILE *f;
   struct PointPairs *pp = NULL;
   pp = (struct PointPairs *)malloc(sizeof(struct PointPairs));

   size_t i, j;
   size_t nmax = 32, skip_count = 0;

   message("Max distance: %7.2f pixels\n", max_pixel_distance);
   pp->pairs = (struct Pair *)realloc(NULL, nmax * sizeof(struct Pair));
   pp->count = 0;

   f = fopen(node_pair_file, "r");
   fscanf(f, "%zu %zu", &i, &j);
   while(!feof(f)) {
      if(i <= npoints && j <= npoints) {
         if(dist(points[i-1], points[j-1]) <= max_pixel_distance) {
            if(pp->count == nmax) {
               nmax *= 2;
               pp->pairs = (struct Pair *)realloc(pp->pairs, nmax * sizeof(struct Pair));
            }
            pp->pairs[pp->count].p1 = points[i-1];
            pp->pairs[pp->count].p2 = points[j-1];
            ++pp->count;
         }
         else {
            ++skip_count;
         }
      }
      else {
         fprintf(stderr, "Invalid pairing (%zu,%zu)\n", i, j);
      }

      fscanf(f, "%zu %zu", &i, &j);
//      while(i > npoints && j > npoints && !feof(f)) {
//         fprintf(stderr, "Invalid pairing (%zu,%zu)\n", i, j);
//         fscanf(f, "%zu %zu", &i, &j);
//      }
   }

   message("%zu pairs generated.  %zu skipped.\n", pp->count, skip_count);
   return pp;
}

static int cmp_pair_close(const void *a, const void *b)
{
   const struct Pair *x = a, *y = b;
   return dist(x->p1, x->p2) < dist(y->p1, y->p2) ? -1 : 1;
}

static int cmp_pair_far(const void *a, const void *b)
{
   const struct Pair *x = a, *y = b;
   return dist(x->p1, x->p2) > dist(y->p1, y->p2) ? -1 : 1;
}

void sort_pairs_close(struct PointPairs *pp)
{
   qsort(pp->pairs, pp->count, sizeof(struct Pair), cmp_pair_close);
}

void sort_pairs_far(struct PointPairs *pp)
{
   qsort(pp->pairs, pp->count, sizeof(struct Pair), cmp_pair_far);
}

// http://stackoverflow.com/a/6127606/7536
void shuffle_pairs(struct PointPairs *pp)
{
   size_t i;
   if(pp->count > RAND_MAX) {
      message("shuffle_pairs cannot handle more than %d pairs", RAND_MAX);
      return;
   }

   srand(time(0));  /* TODO: Should the random seed be a user provided value? */
   for (i = 0; i < pp->count - 1; i++) {
      size_t j = i + rand() / (RAND_MAX / (pp->count - i) + 1);
      struct Pair t = pp->pairs[j];
      pp->pairs[j] = pp->pairs[i];
      pp->pairs[i] = t;
   }
}
