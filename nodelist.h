
#ifndef NODELIST_H
#define NODELIST_H

#include <stdlib.h>
#include "habitat.h"

extern char       node_file[PATH_MAX];
extern char       node_pair_file[PATH_MAX];
extern PetscBool  nearest_first;
extern PetscBool  furthest_first;
extern PetscReal  max_distance;

struct Point
{
   int      index;
   long int x, y;
};

struct Pair { struct Point p1, p2; };
struct PointPairs
{
   struct Pair *pairs;
   size_t count;
   size_t ncount;
};

struct PointPairs *init_point_pairs(struct ResistanceGrid *R);
double dist(struct Point p1, struct Point p2) __attribute__ ((pure));

#endif /* NODELIST_H */
