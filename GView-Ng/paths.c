
#include <stdio.h>
#include <stdlib.h>
#include "paths.h"

char *path_file_name;
struct Paths paths;
extern char *output_file_name;

void readPaths()
{
   FILE     *f;
   size_t    i, j;

   if(path_file_name == NULL) {
      paths.npaths = 0;
      return;
   }

   f = fopen(path_file_name, "r");
   fscanf(f, "%zu %lf %lf", &paths.npaths, &paths.rmin, &paths.rmax);
   paths.paths = (struct Path *)malloc(sizeof(struct Path) * paths.npaths);

   for(i = 0; i < paths.npaths; i++) {
      fscanf(f, "%zu %lf", &paths.paths[i].length, &paths.paths[i].rdist);

      paths.paths[i].xs = (unsigned *)malloc(sizeof(unsigned) * paths.paths[i].length);
      paths.paths[i].ys = (unsigned *)malloc(sizeof(unsigned) * paths.paths[i].length);

      for(j = 0; j < paths.paths[i].length; j++)
         fscanf(f, "%d", &paths.paths[i].xs[j]);

      for(j = 0; j < paths.paths[i].length; j++)
         fscanf(f, "%d", &paths.paths[i].ys[j]);
   }
   fclose(f);
}

void drawPaths(MagickWand *R)
{
   size_t    i, j;
   double    alpha;
   
#define ALPHA_MIN 0.01
#define ALPHA_MAX 0.1
#define THICKNESS 3

   for(i = 0; i < paths.npaths; i++)
   {
      struct Path *pt = &paths.paths[i];
      alpha = (ALPHA_MAX - ALPHA_MIN) * (pt->rdist - paths.rmin) / (paths.rmax - paths.rmin) + ALPHA_MIN;
      for(j = 0; j < pt->length; j++)
      {
         int a,b;
         for(a = -THICKNESS; a < THICKNESS; a++) {
            for(b = -THICKNESS; b < THICKNESS; b++) {
               struct Pixel *p = &R->pixels[pt->ys[j]+a][pt->xs[j]+b];
               p->r = alpha * 0xff + (1 - alpha) * p->r;
               p->g = alpha * 0x69 + (1 - alpha) * p->g;
               p->b = alpha * 0xb4 + (1 - alpha) * p->b;
            }
         }
      }
   }
}

void animatePaths(MagickWand *R)
{
#define TAIL_LEN  100
#define MAX_SPEED 15

   int frame;
   size_t    i, j;
   struct Raster bg; /* make a copy for the background */
   char frame_file_name[80];
   double alpha;
   int all_done;

   frame = 0;
   while(1)
   {
      cloneRaster(&bg, R);

      all_done = 1;
      for(i = 0; i < paths.npaths; i++)
      {
         struct Path *pt = &paths.paths[i];
         int start = frame + MAX_SPEED * (pt->rdist - paths.rmin) / (paths.rmax - paths.rmin);
         alpha = 0.2 * (pt->rdist - paths.rmin) / (paths.rmax - paths.rmin);
         if(start >= pt->length)
            continue;

         for(j = 0; j < TAIL_LEN; j++)
         {
            int a,b;
            if(start < j)
               continue;
            all_done = 0;
            for(a = -THICKNESS; a < THICKNESS; a++) {
               for(b = -THICKNESS; b < THICKNESS; b++) {
                  struct Pixel *p = &bg.pixels[pt->ys[start-j]+a][pt->xs[start-j]+b];
                  p->r = alpha * 0xff + (1 - alpha) * p->r;
                  p->g = alpha * 0x69 + (1 - alpha) * p->g;
                  p->b = alpha * 0xb4 + (1 - alpha) * p->b;
               }
            }
         }
      }
      sprintf(frame_file_name, output_file_name, frame);
      printf("fn = %s\n", frame_file_name);
      saveRaster(&bg, frame_file_name);
      delRaster(&bg);

      frame += 1;
      if(all_done)
         break;
   }
}
