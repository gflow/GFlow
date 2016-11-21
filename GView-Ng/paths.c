
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
   unsigned *xs, *ys;
   size_t    length;

   if(path_file_name == NULL) {
      paths.npaths = 0;
      return;
   }

   f = fopen(path_file_name, "r");
   fscanf(f, "%zu %lf %lf", &paths.npaths, &paths.rmin, &paths.rmax);
   paths.paths = (struct Path *)malloc(sizeof(struct Path) * paths.npaths);

   for(i = 0; i < paths.npaths; i++) {
      struct Path *p = &paths.paths[i];
      int jx, jy;

      fscanf(f, "%zu %lf", &length, &p->rdist);

      xs = (unsigned  *)malloc(sizeof(unsigned) * length);
      ys = (unsigned  *)malloc(sizeof(unsigned) * length);
      p->pi = (PointInfo *)malloc(sizeof(PointInfo) * length);

      for(j = 0; j < length; j++)
         fscanf(f, "%d", &xs[j]);

      for(j = 0; j < length; j++) 
         fscanf(f, "%d", &ys[j]);

      jx = 5 - (rand() % 10);
      jy = 5 - (rand() % 10);
      p->length = 0;
      for(j = 0; j < length; j++) {
         if(j > 0 && xs[j] == xs[j-1] && ys[j] == ys[j-1]) {
            continue;
         }
         p->pi[p->length].x = xs[j] / scale + jx;
         p->pi[p->length].y = ys[j] / scale + jy;
         ++p->length;
      }
      /* FIXME: Shrinking the path vectors can cause duplicates */
 
      free(xs);
      free(ys);
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
   PixelWand    *p = NewPixelWand();
   size_t        i, f;
   size_t       *length, *space, *start;
   double       *speed;
   char          filename[80];

   length = (size_t *)malloc(sizeof(size_t) * paths.npaths);
   space  = (size_t *)malloc(sizeof(size_t) * paths.npaths);
   start  = (size_t *)malloc(sizeof(size_t) * paths.npaths);
   speed  = (double *)malloc(sizeof(double) * paths.npaths);
#define min_length 15
#define max_length 30
#define min_space  50
#define max_space 100
   for(i = 0; i < paths.npaths; i++)
   {
      length[i] = min_length + (rand() % (max_length - min_length));
      space[i]  = min_space  + (rand() % (max_space  - min_space));
      start[i]  = rand() % (length[i] + space[i]);
      speed[i]  = (1. - (paths.paths[i].rdist - paths.rmin) / (paths.rmax - paths.rmin)) * 40. + 1.;
   }

   for(f = 0; f < 24; f++)
   {
      MagickWand  *frame = CloneMagickWand(R);
      DrawingWand *draw = NewDrawingWand();
      PushDrawingWand(draw);

      DrawSetStrokeAntialias(draw, MagickTrue);
      DrawSetStrokeWidth(draw, 2);
      PixelSetColor(p, "#ff69b4");
      PixelSetAlpha(p, 1);
      DrawSetStrokeColor(draw, p);
      DrawSetStrokeOpacity(draw, 0.65);

      DrawSetFillOpacity(draw, 0);
      PixelSetAlpha(p, 0);
      DrawSetFillColor(draw, p);  /* needs to be *something* for opacity=0 to be recognized */

      for(i = 0; i < paths.npaths; i++)
      {
         int j;
         struct Path *pt = &paths.paths[i];
         j = (int)(start[i] + speed[i] * f) % (length[i] + space[i]);
//   printf("strt=%zu length=%zu space=%zu\n", start[i], length[i], space[i]);
         while(j < pt->length) {
            size_t n = (j + length[i]) > pt->length ? pt->length - j : length[i];
            DrawPolyline(draw, n, pt->pi + j);
            j += space[i] + length[i];
//   printf("j=%d n=%zu\n", j, n);
         }
      }
      sprintf(filename, "frame%05zu.png", f);
printf("fn = '%s'\n", filename);
      PopDrawingWand(draw);
      MagickDrawImage(frame, draw);

      MagickWriteImage(frame, filename);
      DestroyDrawingWand(draw);
      DestroyMagickWand(frame);
   }

   free(length);
   free(space);
   free(start);
   free(speed);

   DestroyPixelWand(p);
}
