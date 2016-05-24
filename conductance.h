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
