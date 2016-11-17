
#include <stdio.h>
#include <stdlib.h>
#include "paths.h"

char         *path_file_name;
extern char  *output_file_name;
struct Paths  paths;

void readPaths(double scale)
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
      struct Path *p = &paths.paths[i];

      fscanf(f, "%zu %lf", &paths.paths[i].length, &paths.paths[i].rdist);

      p->xs = (unsigned  *)malloc(sizeof(unsigned)  * p->length);
      p->ys = (unsigned  *)malloc(sizeof(unsigned)  * p->length);
      p->pi = (PointInfo *)malloc(sizeof(PointInfo) * p->length);

      for(j = 0; j < p->length; j++)
         fscanf(f, "%d", &p->xs[j]);

      for(j = 0; j < p->length; j++) 
         fscanf(f, "%d", &p->ys[j]);

      for(j = 0; j < p->length; j++) {
         p->pi[j].x = p->xs[j] / scale;
         p->pi[j].y = p->ys[j] / scale;
      }
      /* FIXME: Shrinking the path vectors can cause duplicates */
 
   }
   fclose(f);
}

void drawPaths(MagickWand *R)
{
   DrawingWand  *draw = NewDrawingWand();
   PixelWand    *p    = NewPixelWand();
   size_t        i;

   PushDrawingWand(draw);
   DrawSetStrokeAntialias(draw, MagickTrue);
   DrawSetStrokeWidth(draw, 1);
   PixelSetColor(p, "#ff69b4");
   PixelSetAlpha(p, 1);
   DrawSetStrokeColor(draw, p);
   DrawSetStrokeOpacity(draw, 0.8);

   DrawSetFillOpacity(draw, 0);
   PixelSetAlpha(p, 0);
   DrawSetFillColor(draw, p);  /* needs to be *something* for opacity=0 to be recognized */

   for(i = 0; i < paths.npaths; i++)
   {
      struct Path *pt = &paths.paths[i];
      DrawPolyline(draw, pt->length, pt->pi);
   }

   PopDrawingWand(draw);
   MagickDrawImage(R, draw);

   DestroyDrawingWand(draw);
   DestroyPixelWand(p);
}

void animatePaths(MagickWand *R)
{
#if 0
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
#endif
}
