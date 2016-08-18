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


#ifndef NODELIST_H
#define NODELIST_H

#include <stdlib.h>
#include "habitat.h"

extern char       node_file[PATH_MAX];
extern char       node_pair_file[PATH_MAX];
extern PetscBool  nearest_first;
extern PetscBool  furthest_first;
extern PetscInt  shuffle_node_pairs;
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
