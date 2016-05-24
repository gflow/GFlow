
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
