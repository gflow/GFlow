
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

extern char      output_directory[PATH_MAX];
extern char      output_prefix[PATH_MAX];
extern int       output_format;
extern double    output_threshold;
extern PetscBool output_final_current_only;
extern PetscBool use_mpiio;

void write_result(struct ResistanceGrid *R,
                  struct ConductanceGrid *G,
                  int index,
                  double *voltages);

void write_total_current(struct ResistanceGrid *R,
                         struct ConductanceGrid *G,
                         int index);

int solution_exists(int index);

void write_effective_resistance(double *voltages, int srcindex,  int srcnode,
                                                  int destindex, int destnode);

void read_complete_solution();
#endif  /* OUTPUT_H */
