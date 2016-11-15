
#ifndef PATHS_H
#define PATHS_H

#include <wand/magick_wand.h>

struct Path
{
   size_t    length;
   unsigned *xs, *ys;
   double    rdist;
};

struct Paths
{
   size_t npaths;
   struct Path *paths;
   double rmin, rmax;
};

void readPaths();
void drawPaths(MagickWand *R);
void animatePaths(MagickWand *R);

#endif /* PATHS_H */
