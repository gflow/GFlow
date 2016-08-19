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

#include "nodelist.h"
#include "habitat.h"
#include "conductance.h"
#include "output.h"
#include "util.h"

#define MPI_SIZE_T MPI_UINT64_T

static PetscReal converge_at = 1.;

static char common_options[] =
   "-ksp_type cg "
/*   "-ksp_monitor_true_residual " */
   "-ksp_atol 1e-12 "
   "-ksp_max_it 1000 "
   "-pc_type hypre "
   "-pc_hypre_type boomeramg ";


static char      habitat_file[PATH_MAX] = { 0 };
static MPI_Comm  COMM_WORKERS;


enum {
   TAG_ROW_RANGE,
   TAG_COL_VALUES,
   TAG_RESULT,
};

struct RowRange
{
   int start, end;
};

struct NodePairSequence
{
   int *seq, count;
};


static void parse_args()
{
   const char *output_formats[3] = {  "asc", "asc.gz", "amp" };
   char convergence[PATH_MAX] = { 0 };
   
   PetscBool flg;
   PetscOptionsGetString(PETSC_NULL, NULL, "-habitat",          habitat_file,     PATH_MAX, &flg);
   PetscOptionsGetString(PETSC_NULL, NULL, "-nodes",            node_file,        PATH_MAX, &flg);
   PetscOptionsGetString(PETSC_NULL, NULL, "-node_pairs",       node_pair_file,   PATH_MAX, &flg);
   PetscOptionsGetString(PETSC_NULL, NULL, "-output_directory", output_directory, PATH_MAX, &flg);
   PetscOptionsGetString(PETSC_NULL, NULL, "-output_prefix",    output_prefix,    PATH_MAX, &flg);
   PetscOptionsGetReal(PETSC_NULL,   NULL, "-output_threshold",&output_threshold,           &flg);
   PetscOptionsGetBool(PETSC_NULL,   NULL, "-use_mpi_io",      &use_mpiio,                  &flg);
   PetscOptionsGetBool(PETSC_NULL,   NULL, "-output_final_current_only",      &output_final_current_only, &flg);
   PetscOptionsGetReal(PETSC_NULL,   NULL, "-max_distance",    &max_distance,               &flg);
   PetscOptionsGetBool(PETSC_NULL,   NULL, "-nearest_first",   &nearest_first,              &flg);
   PetscOptionsGetBool(PETSC_NULL,   NULL, "-furthest_first",  &furthest_first,             &flg);
   PetscOptionsGetInt(PETSC_NULL,   NULL, "-shuffle_node_pairs",  &shuffle_node_pairs,             &flg);
   PetscOptionsGetString(PETSC_NULL, NULL, "-converge_at",      convergence, PATH_MAX, &flg);
   PetscOptionsGetEList(PETSC_NULL,  NULL, "-output_format",
                                     output_formats, 3,  &output_format,              &flg); 

   if(!file_exists(output_directory)) {
      message("Directory `%s` does not exist. Defaulting to current working directory.\n", output_directory);
      output_directory[0] = '.';
      output_directory[1] = 0;
   }
   if(strlen(node_file) > 0 && !file_exists(node_file)) {
      message("%s does not exists\n", node_file);
      MPI_Abort(MPI_COMM_WORLD, 1);
   }
   if(strlen(node_pair_file) > 0 && !file_exists(node_pair_file)) {
      message("%s does not exists\n", node_pair_file);
      MPI_Abort(MPI_COMM_WORLD, 1);
   }
   if(strlen(convergence) > 0) {
      char *p;
      converge_at = strtod(convergence, &p);
      if(p[0] == 'N')
         converge_at = 1. - pow(10., -converge_at);
      if(converge_at < 0. || converge_at > 1.) {
         message("Error.  Convergence factors must be between 0 and 1.\n");
         MPI_Abort(MPI_COMM_WORLD, 1);
      }
      message("Simulation will converge at %lg\n", converge_at);
   }
   read_complete_solution();  /* TODO: Need to remove this feature */
}

static void init_node_pair_sequence(struct NodePairSequence *nps, struct PointPairs *pp)
{
   PetscBool flg;
   int i;
   nps->count = (1<<20) /* pp->count */;
   PetscMalloc(sizeof(int) * nps->count, &nps->seq);
   PetscOptionsGetIntArray(PETSC_NULL, NULL, "-range", nps->seq, &nps->count, &flg);
   if(!flg) {
      nps->count = pp->count;
      for(i = 0; i < nps->count; i++)
         nps->seq[i] = i;
   }
   else {
      i = 0;
      while(i < nps->count) {
         if(0 <= nps->seq[i] && nps->seq[i] < pp->count) {
            i++;
         }
         else {
            message("Removing index %d from pair range.\n", nps->seq[i]);
            nps->seq[i] = nps->seq[nps->count--];
         }
      }
   }
}

static void init_communicator()
{
   int master[1] = { 0 };
   MPI_Group world, workers;
   MPI_Comm_group(MPI_COMM_WORLD, &world);
   MPI_Group_excl(world, 1, master, &workers);
   MPI_Comm_create(MPI_COMM_WORLD, workers, &COMM_WORKERS);
}

static void free_communicator()
{
   if(COMM_WORKERS != MPI_COMM_NULL)
      MPI_Comm_free(&COMM_WORKERS);
}

static void G_add_value(struct ConductanceGrid *G, int x, int y, double value)
{
   int j;
   for(j = 0; G->cols[x*9+j] > -1 && G->cols[x*9+j] != y; j++) { }
   assert(j < 9);
   G->cols[x*9+j] = y;
   G->values[x*9+j] += value;
}

static void update_matrix(struct ResistanceGrid *R, struct ConductanceGrid *G,
                   size_t i, size_t j, off_t a, off_t b)
{
   int id2 = R->cells[i+a][j+b].index;
   if(id2 != -1) {
      double val1 = R->cells[i][j].value;
      double val2 = R->cells[i+a][j+b].value;
      int id1 = R->cells[i][j].index;
      double value = 2. / (val1 + val2);
      if((a&b) != 0) /*a != 0 && b != 0)*/
         value *= M_SQRT1_2;
      if(isinf(value)) {
         message("Infinite value found. R[%zu][%zu] = %lf; R[%llu][%llu] = %lf\n", i, j, val1, i+a, j+b, val2);
      }
      G_add_value(G, id1, id2, -value);
      G_add_value(G, id2, id1, -value);
      G_add_value(G, id1, id1,  value);
      G_add_value(G, id2, id2,  value);
   }
}

static PetscErrorCode init_conductance(struct ResistanceGrid *R, struct ConductanceGrid *G)
{
   int i, j;
   PetscErrorCode ierr;

   message("Number of unknowns: %zu\n", R->cell_count);

   G->nrows = R->cell_count;
   ierr = PetscMalloc(sizeof(int) * G->nrows * 9, &G->cols); CHKERRQ(ierr);
   ierr = PetscMalloc(sizeof(double) * G->nrows * 9, &G->values); CHKERRQ(ierr);
   assert(G->cols != NULL);
   assert(G->values != NULL);

   for(i = 0; i < G->nrows; i++) {
      for(j = 0; j < 9; j++) {
         G->cols[i*9+j] = -1;
         G->values[i*9+j] = 0.;
      }
   }
   for(i = 0; i < R->nrows; i++) {
      for(j = 0; j < R->ncols; j++) {

         if(R->cells[i][j].index == -1)
            continue;

         if(likely(j < R->ncols-1)) 
            update_matrix(R, G, i, j, 0, 1);      /* check east */

         if(likely(i < R->nrows-1)) {
            update_matrix(R, G, i, j, 1, 0);      /* check south */

            if(likely(j > 0))
               update_matrix(R, G, i, j, 1, -1);  /* check south-west */

            if(likely(j < R->ncols-1))
               update_matrix(R, G, i, j, 1, 1);   /* check south-east */
         }
      }
   }

   return 0;
}

static void free_conductance(struct ConductanceGrid *G)
{
   PetscFree(G->cols);
   PetscFree(G->values);
}

static PetscErrorCode init_matrix(Mat *A, size_t count)
{
   int i, wsize;
   PetscInt range[2];
   PetscErrorCode ierr;
   int nrows, *columns;
   double *values;

   MPI_Comm_size(COMM_WORKERS, &wsize);
   ierr = MatCreate(COMM_WORKERS, A);  CHKERRQ(ierr);
   ierr = MatSetSizes(*A, PETSC_DECIDE, PETSC_DECIDE, count, count);  CHKERRQ(ierr);
   ierr = MatSetFromOptions(*A);  CHKERRQ(ierr);
   if(wsize == 1) {
      /* incredibly slow if you use a parallel matrix with one process */
      ierr = MatSeqAIJSetPreallocation(*A, 9, NULL); CHKERRQ(ierr); /* 8 neighbors and self */
   }
   else {
      ierr = MatMPIAIJSetPreallocation(*A, 9, NULL, 9, NULL); CHKERRQ(ierr);
   }
   ierr = MatSetUp(*A);  CHKERRQ(ierr);

   MatGetOwnershipRange(*A, &range[0], &range[1]);
   nrows = range[1] - range[0];
   // message("range = %d - %d\n", range[0], range[1]);

   ierr = PetscMalloc(sizeof(int) * nrows * 9, &columns);   CHKERRQ(ierr);
   ierr = PetscMalloc(sizeof(double) * nrows * 9, &values); CHKERRQ(ierr);

   MPI_Send(range, 2, MPI_INT, 0, TAG_ROW_RANGE, MPI_COMM_WORLD);
   MPI_Recv(columns, nrows * 9, MPI_INT, 0, TAG_COL_VALUES, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
   MPI_Recv(values, nrows * 9, MPI_DOUBLE, 0, TAG_COL_VALUES, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
   // message("Recieved!\n");

   for(i = range[0]; i < range[1]; i++) {
      MatSetValues(*A, 1, &i, 9, &columns[(i-range[0])*9], &values[(i-range[0])*9], INSERT_VALUES);
   }
   ierr = PetscFree(columns);  CHKERRQ(ierr);
   ierr = PetscFree(values);   CHKERRQ(ierr);
   // message("Assembled!\n");

   /* FLUSH required before we can zero out a value */
   ierr = MatAssemblyBegin(*A, MAT_FLUSH_ASSEMBLY);  CHKERRQ(ierr);
   ierr = MatAssemblyEnd(*A, MAT_FLUSH_ASSEMBLY);  CHKERRQ(ierr);

   return 0;
}

static PetscErrorCode solve(Mat *A, int count, int srcnode, int destnode)
{
   int          nrows;
   PetscInt     row_start, row_end;
   PetscScalar  save;
   PetscInt     rhs_indices[2] = { destnode, srcnode };
   PetscScalar  rhs_values[2]  = {      -1.,      1. };
   PetscScalar *result;

   Vec  x, b;
   KSP  ksp;
   PC   pc;
   PetscErrorCode ierr;

   MatGetOwnershipRange(*A, &row_start, &row_end);
   nrows = row_end - row_start;

   ierr = MatAssemblyBegin(*A, MAT_FINAL_ASSEMBLY);  CHKERRQ(ierr);
   ierr = MatAssemblyEnd(*A, MAT_FINAL_ASSEMBLY);    CHKERRQ(ierr);
   if(destnode >= row_start && destnode < row_end) {
      ierr = MatGetValue(*A, destnode, destnode, &save);             CHKERRQ(ierr);
      ierr = MatSetValue(*A, destnode, destnode, 0, INSERT_VALUES);  CHKERRQ(ierr);
    /*  message("Saved value %lf from (%d,%d)\n", save, destnode, destnode); */
   }
   ierr = MatAssemblyBegin(*A, MAT_FINAL_ASSEMBLY);  CHKERRQ(ierr);
   ierr = MatAssemblyEnd(*A, MAT_FINAL_ASSEMBLY);  CHKERRQ(ierr);

   ierr = VecCreate(COMM_WORKERS, &x);  CHKERRQ(ierr);
   ierr = VecSetSizes(x, PETSC_DECIDE, count);  CHKERRQ(ierr);
   ierr = VecSetFromOptions(x);  CHKERRQ(ierr);

   ierr = VecDuplicate(x, &b);  CHKERRQ(ierr);
   ierr = VecSet(b, 0);         CHKERRQ(ierr);
   ierr = VecSetValues(b, 2, rhs_indices, rhs_values, INSERT_VALUES);  CHKERRQ(ierr);
   ierr = VecAssemblyBegin(b);  CHKERRQ(ierr);
   ierr = VecAssemblyEnd(b);    CHKERRQ(ierr);

   ierr = KSPCreate(COMM_WORKERS, &ksp);  CHKERRQ(ierr);
   ierr = KSPSetOperators(ksp, *A, *A);   CHKERRQ(ierr);
   ierr = KSPGetPC(ksp, &pc);             CHKERRQ(ierr);
   ierr = KSPSetFromOptions(ksp);         CHKERRQ(ierr);
   ierr = KSPSetUp(ksp);                  CHKERRQ(ierr);

   ierr = KSPSolve(ksp, b, x);            CHKERRQ(ierr);

   VecGetArray(x, &result);  /* shallow copy */
   MPI_Send(result, nrows, MPI_DOUBLE, 0, TAG_RESULT, MPI_COMM_WORLD);
   VecRestoreArray(x, &result);

   ierr = VecDestroy(&x);    CHKERRQ(ierr);
   ierr = VecDestroy(&b);    CHKERRQ(ierr);
   // ierr = PCDestroy(&pc);   CHKERRQ(ierr);
   ierr = KSPDestroy(&ksp);  CHKERRQ(ierr);

   if(destnode >= row_start && destnode < row_end) {
      ierr = MatSetValue(*A, destnode, destnode, save, INSERT_VALUES);  CHKERRQ(ierr);
   }

   return 0;
}

static void show_eta(double start_time, size_t pos, size_t total)
{
   double tdelta = microtime() - start_time;
   long seconds = (long)(((tdelta * total) / (1+pos)) - tdelta);
   long hours, minutes;
   minutes = seconds / 60;
   seconds %= 60;
   hours = minutes / 60;
   minutes %= 60;
   message("Estimated time remaining: %02ld:%02ld:%02ld\n", hours, minutes, seconds);
}

static int killswitch()
{
   /* First check the working directory */
   if(file_exists("killswitch")) {
      unlink("killswitch");
      return 1;
   }

   return 0;
}

static void manager()
{
   int i, j, index;
   int mpi_size;
   struct PointPairs *pp;
   struct ResistanceGrid R;
   struct ConductanceGrid G;
   struct NodePairSequence nps;
   struct RowRange *ranges;
   double *voltages;
   double start_time;
   int terminate[2] = { -1, -1 };

   MPI_Comm_size(PETSC_COMM_WORLD, &mpi_size);

   parse_args();
   assert(habitat_file != NULL);
   assert(node_file != NULL);
   parse_habitat_file(&R, habitat_file);
   pp = init_point_pairs(&R);
   init_node_pair_sequence(&nps, pp);
   init_conductance(&R, &G);

   if(mpi_size == 1) {
      message("I'm all alone .. bye!\n");
      goto solo_cleanup;
   }

   PetscMalloc(sizeof(struct RowRange) * mpi_size, &ranges);
   MPI_Bcast(&R.cell_count, 1, MPI_SIZE_T, 0, MPI_COMM_WORLD);
   for(i = 1; i < mpi_size; i++) {
      int nrows;
      MPI_Recv(&ranges[i], 2, MPI_INT, i, TAG_ROW_RANGE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      nrows = ranges[i].end - ranges[i].start;
      MPI_Send(&G.cols[ranges[i].start*9], nrows * 9, MPI_INT, i, TAG_COL_VALUES, MPI_COMM_WORLD);
      MPI_Send(&G.values[ranges[i].start*9], nrows * 9, MPI_DOUBLE, i, TAG_COL_VALUES, MPI_COMM_WORLD);
   }

   start_time = microtime();
   voltages = NULL;
   for(i = 0; i < nps.count; i++) {
      double pcoeff = 1;
      index = nps.seq[i];
      int nodes[2]  = { R.cells[pp->pairs[index].p1.x][pp->pairs[index].p1.y].index
                      , R.cells[pp->pairs[index].p2.x][pp->pairs[index].p2.y].index };
      if(nodes[0] == -1) {
         message("Node (%ld,%ld) has zero resistance (most likely).\n", pp->pairs[index].p1.x, pp->pairs[index].p1.y);
         continue;
      }
      if(nodes[1] == -1) {
         message("Node (%ld,%ld) has zero resistance (most likely).\n", pp->pairs[index].p2.x, pp->pairs[index].p2.y);
         continue;
      }
      message("Solving pair %d (%d of %d): %d[%ld,%ld] to %d[%ld,%ld]. %7.2lf Km apart\n",
              index, i+1, nps.count,
              pp->pairs[index].p1.index+1, pp->pairs[index].p1.x, pp->pairs[index].p1.y,
              pp->pairs[index].p2.index+1, pp->pairs[index].p2.x, pp->pairs[index].p2.y,
              dist(pp->pairs[index].p1, pp->pairs[index].p2) * R.cellsize * 1e-3);

      /* inform the worker nodes of the source and destination nodes */
      MPI_Bcast(nodes, 2, MPI_INT, 0, MPI_COMM_WORLD);
      if(voltages != NULL) {
         /* if we have a previous result, save to file (all but first pass through loop) */
         pcoeff = write_result(&R, &G, index-1, voltages);
//         if(index % 100 == 0)
//            write_total_current(&R, &G, index);
      }
      else {
         /* we'll only enter this branch once */
         PetscMalloc(sizeof(double) * G.nrows, &voltages);
      }
      /* wait for workers to solve the linear system, then accept their results */
      for(j = 1; j < mpi_size; j++) {
         int nrows = ranges[j].end - ranges[j].start;
         MPI_Recv(&voltages[ranges[j].start], nrows, MPI_DOUBLE, j, TAG_RESULT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }
      write_effective_resistance(voltages, pp->pairs[index].p1.index, nodes[0],
                                           pp->pairs[index].p2.index, nodes[1]);
      show_eta(start_time, i, nps.count);

      if(pcoeff > converge_at) {
         message("%lf > %lf; converged.\n", pcoeff, converge_at);
         break;
      }
      if(killswitch()) {
         message("Killswitch engaged.\n");
         break;
      }
   }
   /* send the termination singal to the wokers */
   MPI_Bcast(terminate, 2, MPI_INT, 0, MPI_COMM_WORLD);
   /* write the final result */
   write_result(&R, &G, nps.seq[i-1], voltages);
   write_total_current(&R, &G, -1);

   PetscFree(ranges);
   PetscFree(voltages);
solo_cleanup:
   free_habitat(&R);
   free_conductance(&G);
   free(pp->pairs);
   free(pp);
   PetscFree(nps.seq);
}

static void worker()
{
   Mat A;
   size_t count;
   int rank;

   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   MPI_Bcast(&count, 1, MPI_SIZE_T, 0, MPI_COMM_WORLD);
   init_matrix(&A, count);

   while(1) {
      int nodes[2];
      MPI_Bcast(nodes, 2, MPI_INT, 0, MPI_COMM_WORLD);
      if(nodes[0] == -1)
         break;
      solve(&A, count, nodes[0], nodes[1]);
   }
   MatDestroy(&A);
}

int main(int argc, char *argv[])
{
   int rank;

   PetscInitialize(&argc, &argv, NULL, NULL);
   PetscOptionsInsertString(NULL, common_options);
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   init_communicator();
   if(rank == 0)
      manager();
   else 
      worker();

   free_communicator();
   PetscFinalize();
}
