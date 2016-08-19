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


#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <zlib.h>
#include <petsc.h>

#include "output.h"
#include "util.h"

char      output_directory[PATH_MAX] = ".";
char      output_prefix[PATH_MAX] = "I";
int       output_format = OUTPUT_FORMAT_ASC;
double    output_threshold = 1e-9;
PetscBool output_final_current_only = PETSC_FALSE;
PetscBool use_mpiio = PETSC_FALSE;

static float *total_current = NULL;
static float *final_current = NULL;

static void write_asc(struct ResistanceGrid *R,
                      struct ConductanceGrid *G,
                      int index,
                      float *current,
                      PetscBool compress);

static void write_amp(struct ConductanceGrid *G,
                      int index,
                      float *current);

static float *calculate_current(struct ConductanceGrid *G, double *voltages);

static double pearson_coefficient(size_t n, float *x, float *w);
static double sum_sqr(size_t n, float *x, float *w);
static double rsme(size_t n, float *x, float *w);
static int    nines(double x);

double write_result(struct ResistanceGrid *R,
                    struct ConductanceGrid *G,
                    int index,
                    double *voltages)
{
   float *current, *prev_total;
   double pcoeff;
   int    i;

   current = calculate_current(G, voltages);
   if(current) {
      if(!output_final_current_only) {
         switch(output_format)
         {
            case OUTPUT_FORMAT_ASC:
               write_asc(R, G, index, current, PETSC_FALSE);
               break;
            case OUTPUT_FORMAT_ASC_GZ:
               write_asc(R, G, index, current, PETSC_TRUE);
               break;
            case OUTPUT_FORMAT_AMP:
               write_amp(G, index, current);
               break;
         }
      }
      else {
         message("Solution to iteration %d discarded.\n", index);
      }

      if(total_current == NULL) {
         PetscMalloc(sizeof(float) * G->nrows, &total_current);
         memset(total_current, 0, sizeof(float) * G->nrows);
      }
      prev_total = total_current;
      PetscMalloc(sizeof(float) * G->nrows, &total_current);
      for(i = 0; i < G->nrows; i++) {
         total_current[i] = current[i] + prev_total[i];
      }
      pcoeff = pearson_coefficient(G->nrows, total_current, prev_total);
      message("convergence-factor = %e (%d-N)\n", pcoeff, nines(pcoeff));
      if(final_current) {
         double p = pearson_coefficient(G->nrows, final_current, total_current);
         message("correlation = %e\n", p);
      }
      PetscFree(current);
      PetscFree(prev_total);
      return pcoeff;
   }
   return 1;  /* need to return something so we don't converge early */
}

void write_total_current(struct ResistanceGrid *R,
                         struct ConductanceGrid *G,
                         int index)
{

   if(total_current == NULL)
      return;

   switch(output_format)
   {
      case OUTPUT_FORMAT_ASC:
         write_asc(R, G, index, total_current, PETSC_FALSE);
         break;
      case OUTPUT_FORMAT_ASC_GZ:
         write_asc(R, G, index, total_current, PETSC_TRUE);
         break;
      case OUTPUT_FORMAT_AMP:
         write_amp(G, index, total_current);
         break;
   }
   if(index == -1)
      PetscFree(total_current);
}

int solution_exists(int index)
{
   char name[80];
   sprintf(name, "%s/%s%06d.", output_directory, output_prefix, index);
   switch(output_format)
   {
      case OUTPUT_FORMAT_ASC:
         strcat(name, "asc");
         break;
      case OUTPUT_FORMAT_ASC_GZ:
         strcat(name, "asc.gz");
         break;
      case OUTPUT_FORMAT_AMP:
         strcat(name, "amp");
   }
   return file_exists(name);
}

void write_asc(struct ResistanceGrid *R,
               struct ConductanceGrid *G,
               int index,
               float *current,
               PetscBool compress)
{
   char    fname[80];
   void   *fout;  /* will either be FILE or gzFile */
   int     i, gx, gy;

   typedef void *(*file_open_func)(const char *, const char *);
   typedef int   (*file_printf_func)(void *, const char *, ...);
   typedef int   (*file_close_func)(void *);
   file_open_func   file_open;
   file_printf_func file_printf;
   file_close_func  file_close;

   if(index == -1)
      sprintf(fname, "%s/%sfinal.asc", output_directory, output_prefix);
   else
      sprintf(fname, "%s/%s%06d.asc", output_directory, output_prefix, index);
   if(compress) {
      strcat(fname, ".gz");
      file_open = (file_open_func)gzopen;
      file_printf = (file_printf_func)gzprintf;
      file_close = (file_close_func)gzclose;
   }
   else {
      file_open = (file_open_func)fopen;
      file_printf = (file_printf_func)fprintf;
      file_close = (file_close_func)fclose;
   }

   fout = file_open(fname, "w");
   file_printf(fout, "ncols %d\n", R->ncols);
   file_printf(fout, "nrows %d\n", R->nrows);
   file_printf(fout, "xllcorner %lf\n", R->xllcorner);
   file_printf(fout, "yllcorner %lf\n", R->yllcorner);
   file_printf(fout, "cellsize %d\n", (int)R->cellsize);
   file_printf(fout, "NODATA_value %d\n", (int)R->NODATA_value);

   gx = gy = 0;
   for(i = 0; i < G->nrows; i++) {

      while(R->cells[gx][gy].index == -1) {
         file_printf(fout, "-9999 ");
         if(++gy == R->ncols) {
            file_printf(fout, "\n");
            gy = 0;
            ++gx;
         }
      }

      file_printf(fout, "%f ", current[i]);
      if(++gy == R->ncols) {
         file_printf(fout, "\n");
         gy = 0;
         ++gx;
      }
   }

   for(; gx < R->nrows; gx++) {
      for(; gy < R->ncols; gy++)
         file_printf(fout, "-9999 ");
      file_printf(fout, "\n");
   }
   file_close(fout);
   message("Result %s written.\n", fname);
}

void write_amp(struct ConductanceGrid *G,
               int index,
               float *current)
{
   char    fname[80];
   gzFile *fout;

   if(index == -1)
      sprintf(fname, "%s/%sfinal.amp", output_directory, output_prefix);
   else
      sprintf(fname, "%s/%s%06d.amp", output_directory, output_prefix, index);

   fout = gzopen(fname, "w");
   gzwrite(fout, &G->nrows, sizeof(int));
   gzwrite(fout, current, sizeof(float) * G->nrows);
   gzclose(fout);
   message("Result %s written.\n", fname);
}

void write_effective_resistance(double *voltages, int srcindex,  int srcnode,
                                                  int destindex, int destnode)
{
   // V = IR;  I = 1A;  R = \delta{}V
   message("R_eff = %d,%d,%lf\n", srcindex+1, destindex+1, voltages[srcnode] - voltages[destnode]);
}

float *calculate_current(struct ConductanceGrid *G, double *voltages)
{
   int     i, j;
   float  *current;

   PetscMalloc(sizeof(float) * G->nrows, &current);
   for(i = 0; i < G->nrows; i++) {
      double pos = 0;
      double neg = 0;

      for(j = 0; G->cols[i*9+j] != -1 && j < 9; j++) {
         
         if(G->values[i*9+j] < 0) {
           double amps = -G->values[i*9+j] * (voltages[i] - voltages[G->cols[i*9+j]]);
           if(amps < 0)
              neg += -amps;
           else
              pos +=  amps;
         }
      }
      current[i] = (float)(fmax(pos, neg) < output_threshold ? 0. : fmax(pos, neg));
   }
   return current;
}

double pearson_coefficient(size_t n, float *x, float *y)
{
   double Sxy = 0.  /* sum of xi*yi */
        , Sx  = 0.  /* sum of xi    */
        , Sy  = 0.  /* sum of yi    */
        , Sx2 = 0.  /* sum of xi^2  */
        , Sy2 = 0.; /* sum of yi^2  */
   size_t i;

   for(i = 0; i < n; i++) {
      Sxy += x[i] * y[i];
      Sx  += x[i];
      Sy  += y[i];
      Sx2 += x[i] * x[i];
      Sy2 += y[i] * y[i];
   }
   return (Sy2 > 0) ? (n * Sxy - Sx*Sy) / (sqrt(n * Sx2 - Sx*Sx) * sqrt(n * Sy2 - Sy*Sy)) : 0.;
}

double sum_sqr(size_t n, float *x, float *y)
{
   double sum = 0;
   size_t i;
   for(i = 0; i < n; i++)
      sum += (x[i] - y[i]) * (x[i] - y[i]);
   return sum;
}

double rsme(size_t n, float *x, float *y)
{
   return sqrt(sum_sqr(n, x, y) / n);
}

int nines(double x)
{
   return (int)floor(fabs(log10(1-x)));
}

// I hope to delete this section ASAP
void read_complete_solution()
{
   char solfile[PATH_MAX] = { 0 };
   PetscBool flg;
   gzFile f;
   int count;

   PetscOptionsGetString(PETSC_NULL, NULL, "-complete_solution", solfile, PATH_MAX, &flg);
   if(flg) {
      message("Reading complete solution from %s\n", solfile);
      f = gzopen(solfile, "r");
      gzread(f, &count, sizeof(int));
      final_current = (float *)malloc(sizeof(float) * count);
      gzread(f, final_current, sizeof(float) * count);
      gzclose(f);
   }
}
