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


#ifndef HABITAT_H
#define HABITAT_H

struct RCell
{
   double value;
   size_t index;
};

struct ResistanceGrid
{
   int    ncols, nrows;
   double xllcorner, yllcorner;
   double cellsize, NODATA_value;
   size_t cell_count;
   struct RCell **cells;
};

void parse_habitat_file(struct ResistanceGrid *R, const char *habitat_file);
void free_habitat(struct ResistanceGrid *R);

#endif  /* HABITAT_H */
