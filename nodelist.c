
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <petsc.h>

#include "util.h"
#include "nodelist.h"

char       node_file[PATH_MAX] = { 0 };
char       node_pair_file[PATH_MAX] = { 0 };
PetscBool  nearest_first = PETSC_FALSE;
PetscBool  furthest_first = PETSC_FALSE;
PetscReal  max_distance = 40e6;  /* circumference of the earth (approx) */
PetscBool  resistance_only = PETSC_FALSE;

static struct Point *parse_node_list(char *filename, size_t *npoints);
static int validate_points(struct Point *points, size_t npoints, struct ResistanceGrid *R);
static struct PointPairs *generate_pairs(struct Point *points, size_t npoints, double max_pixel_distance);
static struct PointPairs *parse_node_pair_file(struct Point *points, size_t npoints, double max_pixel_distance);
static void sort_pairs_close(struct PointPairs *pairs);
static void sort_pairs_far(struct PointPairs *pairs);

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

   return pp;
}

int validate_points(struct Point *points, size_t npoints, struct ResistanceGrid *R)
{
   size_t i;
   int result = 1;
   for(i = 0; i < npoints; i++) {
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

struct Point *parse_node_list(char *filename, size_t *npoints)
{
   struct Point *points = NULL;
   size_t nmax = 32;
   FILE *f = fopen(filename, "r");

   *npoints = -1;
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

