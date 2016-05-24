
#ifndef CONDUCTANCE_H
#define CONDUCTANCE_H

// TODO: Update G to use these types.  It'll be cleaner
typedef int G_cols_t[10];
typedef double G_values_t[9];

struct ConductanceGrid
{
   int nrows;           /* number of rows in the grid */
   int   *cols;    /* the column indicies */
   double *values;  /* all the values, 9xN */
};

#endif  /* CONDUCTANCE_H */
