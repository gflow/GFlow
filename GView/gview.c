
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
/* #include <linux/limits.h> */

#include <assert.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <float.h>
#include <math.h>
#include <zlib.h>


#define streq(X,Y) (strcmp((X),(Y))==0)

struct Pixel {
   unsigned char r, g, b;
};

static struct {
   size_t count;
   struct Pixel *scale;
} ColorScale = { 0, NULL };

static char *input_file_name = NULL;
static char *input_mask_name = NULL;
static char *output_file_name = NULL;
static int   exp_scale = 0;


void parse_grid(char *gfile);
void set_color_theme(char *theme);

void message(const char *fmt, ...)
{
   char buff[80];
   time_t rawtime;
   struct tm *tminfo;
   va_list ap;

   time(&rawtime);
   tminfo = localtime(&rawtime);
   strftime(buff, 80, "%c", tminfo);
   fprintf(stderr, "%s >> ", buff);
   va_start(ap, fmt);
   vfprintf(stderr, fmt, ap);
   va_end(ap);
}

void usage(char *exe)
{

}

void parseargs(int argc, char *argv[])
{
   static struct option long_options[] = {
      { "help",     no_argument,       0, 'h' },
      { "mask",     required_argument, 0, 'm' },
      { "output",   required_argument, 0, 'o' },
      { "colors",   required_argument, 0, 'c' },
      { "exp-scale",no_argument,       0, 'e' },
      { 0,          0,                 0,  0  } };

  while(1) {
      int option_index;
      int c = getopt_long(argc, argv, "hc:o:em:", long_options, &option_index);

      if(c == -1)
         break;

      switch(c) {
         case 'h':
            usage(argv[0]);
         case 'c':
            set_color_theme(optarg);
            break;
         case 'm':
            input_mask_name = strdup(optarg);
            break;
         case 'o':
            output_file_name = strdup(optarg);
            break;
         case 'e':
            exp_scale = 1;
            break;
      }
   }
   input_file_name = strdup(argv[optind]); 
}

int file_exists(const char *path)
{
   return access(path, R_OK) == 0;
}

long file_size(const char *path)
{
  struct stat buf;
  if(stat(path, &buf) == -1)
    return -1;
  return buf.st_size;
}

static char *get_file_handle(const char *filename, long *fsz)
{
   int     fd;
   void   *handle;

   if(!file_exists(filename)) {
      fprintf(stderr, "%s does not exist\n", filename);
      exit(1);
   }
   *fsz = file_size(filename);
   fd = open(filename, O_RDONLY);
   assert(fd > 0);
   handle = mmap(NULL, *fsz, PROT_READ, MAP_PRIVATE, fd, 0);
   if(handle == MAP_FAILED) {
      fprintf(stderr, "Error mapping %s to memory", filename);
      exit(1);
   }
   close(fd);
   return (char *)handle;
}

double **create_grid(int nrows, int ncols)
{
   int i;
   double *data = malloc(sizeof(double) * nrows * ncols);
   double **result = malloc(sizeof(double *) * nrows);
   for(i = 0; i < nrows; i++) {
      result[i] = &data[i * ncols];
   }
   return result;
}

struct Pixel **create_raster(int nrows, int ncols)
{
   int i;
   struct Pixel *data = malloc(sizeof(struct Pixel) * nrows * ncols);
   struct Pixel **result = malloc(sizeof(struct Pixel *) * nrows);
   for(i = 0; i < nrows; i++) {
      result[i] = &data[i * ncols];
   }
   return result;
}

static float *parse_amp(const char *fname)
{
   gzFile *f;
   int count;
   float *values;
   message("amp: %s\n", fname);
   f = gzopen(fname, "r");
   gzread(f, &count, sizeof(int));
   message("count = %d\n", count);
   values = (float *)malloc(sizeof(float) * count);
   gzread(f, values, sizeof(float) * count);
   gzclose(f);
   return values;
}

static char *parse_header(char    *grid,
                          int     *ncols,     int    *nrows,
                          double  *xllcorner, double *yllcorner,
                          double  *cellsize,  double *NODATA_value)
{
   char key[32];
   int  bytes;

   while(isalpha(grid[0])) {
       sscanf(grid, "%s%n", key, &bytes);
       grid += bytes;
   
       if(streq(key,"ncols"))
         sscanf(grid, "%d%n", ncols, &bytes);
       else if(streq(key,"nrows"))
         sscanf(grid, "%d%n", nrows, &bytes);
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
   message("(row,cols) = (%d,%d)\n", *nrows, *ncols);
   return grid;
}

void parse_input(const char *grid_file)
{
   long fsz;
   char *rh = get_file_handle(grid_file, &fsz);
   parse_grid(rh);
   munmap(rh, fsz);
}

void getPixel(struct Pixel p, double *r, double *g, double *b)
{
   *r = (double)p.r / 255.;
   *g = (double)p.g / 255.;
   *b = (double)p.b / 255.;
}

void parse_grid(char *gfile)
{
   int     ncols, nrows;
   double  xllcorner, yllcorner;
   double  cellsize,  NODATA_value;
   char   *p, *q;
   int     i, j, k;
   double  **grid;
   double  hi, lo;

   struct Pixel **raster;
   double marks[25] = { -1 };
   FILE *fout;
   float *amp = NULL, *aptr = NULL;

   p = parse_header(gfile,
                    &ncols, &nrows,
                    &xllcorner, &yllcorner,
                    &cellsize,  &NODATA_value);
   grid = create_grid(nrows, ncols);

   if(input_mask_name) {
      aptr = amp = parse_amp(input_file_name);
   } else message("Grid is data (not mask)\n");
   hi = DBL_MIN;
   lo = DBL_MAX;
   for(i = 0; i < nrows; i++) {
      for(j = 0; j < ncols; j++) {
         grid[i][j] = strtod(p, &q);
         p = q;
         //if(grid[i][j] != NODATA_value) {
         if(grid[i][j] > 0) {
            if(aptr) grid[i][j] = *aptr++;
            hi = fmax(hi, grid[i][j]);
            lo = fmin(lo, grid[i][j]);
         }
      }
   }
   if(amp) free(amp);
   message("range: (%f,%f)\n", lo, hi);

   message("num colors = %d\n", ColorScale.count);
   if(exp_scale) {
      marks[0] = 0.;
      for(k = 1; k < ColorScale.count; k++)
         marks[k] = 1. / (1 << (ColorScale.count - k - 1));
   }
   else {
      for(k = 0; k < ColorScale.count; k++)
         marks[k] = (double)k / (ColorScale.count - 1);
   }
   for(k = 0; k < ColorScale.count; k++)
      message("mark %d = %lf\n", k, marks[k]);
 
   raster = create_raster(nrows, ncols);
   for(i = 0; i < nrows; i++) {
      for(j = 0; j < ncols; j++) {
         double ratio1, ratio2;

         if(grid[i][j] == NODATA_value) {
            raster[i][j].r = 200;
            raster[i][j].g = 200;
            raster[i][j].b = 200;
         }
         else {
            double r1, g1, b1;
            double r2, g2, b2;
            ratio1 = (grid[i][j] - lo) / (hi - lo);
            for(k = 0; marks[k+1] < ratio1; k++) { }
            ratio2 = (ratio1 - marks[k]) / (marks[k+1] - marks[k]);
            getPixel(ColorScale.scale[k],   &r1, &g1, &b1);
            getPixel(ColorScale.scale[k+1], &r2, &g2, &b2);
            raster[i][j].r = 255 * ((r2 - r1) * ratio2 + r1);
            raster[i][j].g = 255 * ((g2 - g1) * ratio2 + g1);
            raster[i][j].b = 255 * ((b2 - b1) * ratio2 + b1);
         }
      }
   }

   fout = streq(output_file_name, "-") ? stdout : fopen(output_file_name, "w");
   fprintf(fout, "P6 %d %d 255\n", ncols, nrows);
   for(i = 0; i < nrows; i++) {
     fwrite(raster[i], sizeof(struct Pixel), ncols, fout);
   }
   fclose(fout);

}

unsigned char get_channel(char *p)
{
   unsigned char a = p[0] >= 'a' ? p[0]-'a'+10 : p[0]-'0';
   unsigned char b = p[1] >= 'a' ? p[1]-'a'+10 : p[1]-'0';
   return (a*16 + b);
}

void set_color_scale(char *colors)
{
   char *p;
   int   i = 0;

   ColorScale.count = (strlen(colors) + 1) / 7;
   ColorScale.scale = (struct Pixel *)malloc(sizeof(struct Pixel) * ColorScale.count);
   for(p = strtok(strdup(colors), ","); p; p = strtok(NULL, ",")) {
      ColorScale.scale[i].r = get_channel(&p[0]);
      ColorScale.scale[i].g = get_channel(&p[2]);
      ColorScale.scale[i].b = get_channel(&p[4]);
      i++;
   }
}

void set_color_theme(char *theme)
{
   message("Color theme: %s\n", theme);
   if(streq(theme, "rgb"))
      set_color_scale("0000ff,00ff00,ff0000");
   else if(streq(theme, "warm"))
      set_color_scale("000000,ff0000,ff6600,ffff99");
   else if(streq(theme, "cool"))
      set_color_scale("008000,333399,81007f");
   else if(streq(theme, "rainbow"))
      set_color_scale("ff0000,ff6600,ffff99,00ff00,0000ff,4b0082,663399");
   else if(streq(theme, "topo"))
      set_color_scale("000000,0019ff,0080ff,00e5ff,00ff4d,19fe00,80ff00,e6ff00,ffff00,ffe53c,ffdb77,ffe0b2");
   else if (streq(theme,"viridis"))
      set_color_scale("000000,440154ff,481568ff,482677ff,453781ff,3f4788ff,39558cff,32648eff,2d718eff,287d8eff,38a8dff,1f968bff,20a386ff,29af7fff,3cbc75ff,56c667ff,74d055ff,94d840ff,b8de29ff,dce318ff,fde725ff");
   else if (streq(theme,"viridis50"))
      set_color_scale("000000,440154ff, 46085cff, 471064ff, 48176aff, 481f70ff, 482576ff, 472c7aff, 46337eff, 443983ff,423f85ff, 404588ff, 3e4a89ff, 3c508bff, 39568cff, 365c8dff, 34618dff, 31668eff, 2f6b8eff,2d718eff, 2b758eff, 297a8eff, 277f8eff, 25848eff, 23898eff, 218e8dff, 20928cff, 1f978bff,1e9d89ff, 1fa187ff, 21a685ff, 25ab82ff, 29af7fff, 30b57cff, 38b977ff, 40bd72ff, 4ac16dff,55c568ff, 5fca61ff, 6bcd5aff, 77d153ff, 84d44bff, 91d742ff, 9fda3aff, acdc30ff, bade28ff,c8e020ff, d6e21aff, e4e419ff, f1e51dff, fde725ff");
   else if (streq(theme,"viridis35"))
      set_color_scale("000000,440154FF, 460C5FFF, 481769FF, 482072FF, 472A7AFF, 463380FF, 433D84FF, 404588FF, 3D4E8AFF,39558CFF, 355E8DFF, 32668EFF, 2E6D8EFF, 2B738EFF, 297B8EFF, 26828EFF, 23898EFF, 21908CFF,1F978BFF, 1F9F89FF, 21A585FF, 25AD82FF, 2EB37CFF, 39BA77FF, 46C06FFF, 55C568FF, 65CB5EFF,76D054FF, 89D548FF, 9CD93BFF, B0DD2FFF, C3E022FF, D8E219FF, EBE51BFF FDE725FF");
   else if (streq(theme,"topo64"))
      set_color_scale("000000,010107,03030E,040415,06061C,070723,09092A,0B0B31,0C0C38,0E0E40,0F0F47,11114E,131355,14145C,161663,17176A,181872,17177B,151584,13138D,121296,10109F,0F0FA8,0D0DB1,0B0BBA,0A0AC3,0808CD,0707D6,0505DF,0303E8,0202F1,0000FA,0008FF,0018FF,0028FF,0038FF,0048FF,0059FF,0069FF,0079FF,0089FF,0099FF,00A9FF,00BAFF,00CAFF,00DAFF,00EAFF,00FAFF,00FFF2,00FFE2,00FFD2,00FFC2,00FFB2,00FFA1,00FF91,00FF81,00FF71,00FF61,00FF50,00FF40,00FF30,00FF20,00FF10,00FF00");   
   else
      set_color_scale(theme);  /* assume it's user-defined colors */
}

int main(int argc, char *argv[])
{
   parseargs(argc, argv);
   if(ColorScale.count == 0) 
      set_color_theme("rgb");

   parse_input(input_mask_name ?: input_file_name);
   return 0;
}
