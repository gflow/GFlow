
#include <assert.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <wand/magick_wand.h>

#include "fileutil.h"
#include "colors.h"
#include "image.h"

#define streq(X,Y) (strcmp((X),(Y))==0)

extern char *input_file_name;

static double **createGrid(int nrows, int ncols)
{
   int i;
   double *data = malloc(sizeof(double) * nrows * ncols);
   double **result = malloc(sizeof(double *) * nrows);
   for(i = 0; i < nrows; i++) {
      result[i] = &data[i * ncols];
   }
   return result;
}

static void getPixel(struct RGB p, double *r, double *g, double *b)
{
   *r = (double)p.r / 255.;
   *g = (double)p.g / 255.;
   *b = (double)p.b / 255.;
}

static char *parseHeader(char   *grid,
                         size_t *ncols,     size_t *nrows,
                         double *xllcorner, double *yllcorner,
                         double *cellsize,  double *NODATA_value)
{
   char key[32];
   int  bytes;

   while(isalpha(grid[0])) {
       sscanf(grid, "%s%n", key, &bytes);
       grid += bytes;
   
       if(streq(key,"ncols"))
         sscanf(grid, "%zu%n", ncols, &bytes);
       else if(streq(key,"nrows"))
         sscanf(grid, "%zu%n", nrows, &bytes);
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

static MagickWand *parseGrid(char *file_contents)
{
   size_t  ncols, nrows;
   double  xllcorner, yllcorner;
   double  cellsize, NODATA_value;
   char   *p, *q;
   size_t     i, j, k;
   double  **grid;
   double  hi, lo;
   double  marks[25] = { -1 };

   MagickWand     *R        = NULL;
   PixelWand      *pixel    = NULL;
   PixelIterator  *iterator = NULL;
   PixelWand     **pixels   = NULL;


   p = parseHeader(file_contents,
                   &ncols,     &nrows,
                   &xllcorner, &yllcorner,
                   &cellsize,  &NODATA_value);

   fprintf(stderr, "Grid size: %zu x %zu\n", ncols, nrows);
   grid = createGrid(nrows, ncols);

   lo = DBL_MAX;
   hi = 0.;
   for(i = 0; i < nrows; i++) {
      for(j = 0; j < ncols; j++) {
         grid[i][j] = strtod(p, &q);
         p = q;
         if(grid[i][j] > 0) {
            hi = fmax(hi, grid[i][j]);
            lo = fmin(lo, grid[i][j]);
         }
      }
   }
   fprintf(stderr, "range: (%f,%f)\n", lo, hi);

   fprintf(stderr, "num colors = %zu\n", ColorScale.count);
   marks[0] = 0.;
   for(k = 1; k < ColorScale.count; k++)
      marks[k] = 1. / (1 << (ColorScale.count - k - 1));

   for(k = 0; k < ColorScale.count; k++)
      fprintf(stderr, "mark %zu = %lf\n", k, marks[k]);
 
   pixel = NewPixelWand();
   PixelSetColor(pixel, "#c8c8c8");
   R = NewMagickWand();
   MagickNewImage(R, ncols, nrows, pixel);

   iterator = NewPixelIterator(R);
   for(i = 0; i < nrows; i++) {
      pixels = PixelGetNextIteratorRow(iterator, &j);
      for(j = 0; j < ncols; j++) {
         double ratio1, ratio2;
         double r1, g1, b1;
         double r2, g2, b2;

         if(grid[i][j] == NODATA_value)
            continue;

         ratio1 = (grid[i][j] - lo) / (hi - lo);
         for(k = 0; marks[k+1] < ratio1; k++) { }
         ratio2 = (ratio1 - marks[k]) / (marks[k+1] - marks[k]);
         getPixel(ColorScale.scale[k],   &r1, &g1, &b1);
         getPixel(ColorScale.scale[k+1], &r2, &g2, &b2);

         PixelSetRed(pixels[j],  (r2 - r1) * ratio2 + r1);
         PixelSetGreen(pixels[j],(g2 - g1) * ratio2 + g1);
         PixelSetBlue(pixels[j], (b2 - b1) * ratio2 + b1);
      }
      PixelSyncIterator(iterator);
   }
   DestroyPixelIterator(iterator);

   free(grid[0]);
   free(grid);

   return R;
}

MagickWand *loadImage()
{
   long fsz;
   char *rh = get_file_handle(input_file_name, &fsz);
   MagickWand *result = parseGrid(rh);
   munmap(rh, fsz);
   return result;
}

