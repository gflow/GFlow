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


#ifndef OUTPUT_H
#define OUTPUT_H

#include "conductance.h"
#include "habitat.h"
#include "nodelist.h"

enum {
   OUTPUT_FORMAT_ASC,
   OUTPUT_FORMAT_ASC_GZ,
   OUTPUT_FORMAT_AMP,
};

extern char      output_density_filename[PATH_MAX];
extern char      output_sum_density_filename[PATH_MAX];
extern char      output_max_density_filename[PATH_MAX];
extern double    output_threshold;
extern char      reff_path[PATH_MAX];
extern PetscBool use_mpiio;

double write_result(struct ResistanceGrid *R,
                    struct ConductanceGrid *G,
                    unsigned long iter,
                    unsigned long src,
                    unsigned long dest,
                    double *voltages);

void write_total_current(struct ResistanceGrid *R,
                         struct ConductanceGrid *G,
                         int index);

void write_effective_resistance(double *voltages, int srcindex,  int srcnode,
                                                  int destindex, int destnode);

void read_complete_solution();
#endif  /* OUTPUT_H */
