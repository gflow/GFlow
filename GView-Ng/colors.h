
#ifndef COLORS_H
#define COLORS_H

struct RGB
{
   unsigned char r, g, b;
};

struct ColorScaleType
{
   size_t count;
   struct RGB *scale;
};

extern struct ColorScaleType ColorScale;

void set_color_theme(char *theme);

#endif /* COLORS_H */
