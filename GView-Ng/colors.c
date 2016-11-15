
#include <stdlib.h>
#include <string.h>
#include "colors.h"

#define streq(X,Y) (strcmp((X),(Y))==0)

struct ColorScaleType ColorScale = { 0, NULL };

static unsigned char get_channel(char *p)
{
   unsigned char a = p[0] >= 'a' ? p[0]-'a'+10 : p[0]-'0';
   unsigned char b = p[1] >= 'a' ? p[1]-'a'+10 : p[1]-'0';
   return (a*16 + b);
}

static void set_color_scale(char *colors)
{
   char *p;
   int   i = 0;

   ColorScale.count = (strlen(colors) + 1) / 7;
   ColorScale.scale = (struct RGB *)malloc(sizeof(struct RGB) * ColorScale.count);
   for(p = strtok(strdup(colors), ","); p; p = strtok(NULL, ",")) {
      ColorScale.scale[i].r = get_channel(&p[0]);
      ColorScale.scale[i].g = get_channel(&p[2]);
      ColorScale.scale[i].b = get_channel(&p[4]);
      i++;
   }
}

void set_color_theme(char *theme)
{
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
   else if(streq(theme, "viridis"))
      set_color_scale("000000,440154,481568,482677,453781,3f4788,39558c,32648e,2d718e,287d8e,38a8d,1f968b,20a386,29af7f,3cbc75,56c667,74d055,94d840,b8de29,dce318,fde725");
   else if (streq(theme, "viridis50"))
      set_color_scale("000000,440154,46085c,471064,48176a,481f70,482576,472c7a,46337e,443983,423f85,404588,3e4a89,3c508b,39568c,365c8d,34618d,31668e,2f6b8e,2d718e,2b758e,297a8e,277f8e,25848e,23898e,218e8d,20928c,1f978b,1e9d89,1fa187,21a685,25ab82,29af7f,30b57c,38b977,40bd72,4ac16d,55c568,5fca61,6bcd5a,77d153,84d44b,91d742,9fda3a,acdc30,bade28,c8e020,d6e21a,e4e419,f1e51d,fde725");
   else if(streq(theme, "viridis35"))
      set_color_scale("000000,440154,460c5f,481769,482072,472a7a,463380,433d84,404588,3d4e8a,39558c,355e8d,32668e,2e6d8e,2b738e,297b8e,26828e,23898e,21908c,1f978b,1f9f89,21a585,25ad82,2eb37c,39ba77,46c06f,55c568,65cb5e,76d054,89d548,9cd93b,b0dd2f,c3e022,d8e219,ebe51b,fde725");
   else if(streq(theme, "topo64"))
      set_color_scale("000000,010107,03030e,040415,06061c,070723,09092a,0b0b31,0c0c38,0e0e40,0f0f47,11114e,131355,14145c,161663,17176a,181872,17177b,151584,13138d,121296,10109f,0f0fa8,0d0db1,0b0bba,0a0ac3,0808cd,0707d6,0505df,0303e8,0202f1,0000fa,0008f,0018f,0028ff,0038ff,0048ff,0059ff,0069ff,0079ff,0089ff,0099ff,00a9ff,00baff,00caff,00daff,00eaff,00faff,00fff2,00ffe2,00ffd2,00ffc2,00ffb2,00ffa1,00ff91,00ff81,00ff71,00ff61,00ff50,00ff40,00ff30,00ff20,00ff10,00ff00");   
   else
      set_color_scale(theme);  /* assume it's user-defined colors */
}

