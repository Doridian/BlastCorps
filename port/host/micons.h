/* --model-icons (micons.c): the game's pictures of its models (the icons
   of the hint panels, the levels' goals, the vehicles) drawn as the models
   themselves */
#ifndef MICONS_H
#define MICONS_H

#include <stdint.h>

extern int micons_on;                   /* --model-icons / PORT_MODEL_ICONS */
extern unsigned micons_serial;          /* bumped when the pictures change */

/* the picture whose texture is at addr (an N64 address, a draw's
   G_SETTIMG): a handle for micons_part, or -1 */
int micons_lookup(uint32_t addr);

/* row h of a picture drawn over x0..x1, y0..y1 (screen pixels): the drop
   shadow (shadow, in colour prim) or the picture (alpha prim[3]).  Once
   the picture's last row is in, the display list that draws its model
   there (an N64 address for gfx.c to run), else 0. */
uint32_t micons_part(int h, float x0, float y0, float x1, float y1, int shadow, const uint8_t prim[4],
                     uint32_t cimg, int cimg_siz, int cimg_w, uint32_t zimg, const int scissor[4]);

/* the world map's chopper (its picture on a quad lying on the globe): the
   display list that draws its model about the quad's middle c (the
   globe's space), under the game's modelview mv, with the picture's
   alpha.  first: the frame's first pass, which moves it on; else (an
   in-between pass of --interpolate) the first's list again. */
uint32_t micons_globe(const float c[3], const float mv[4][4], int first, uint8_t alpha, uint32_t cimg,
                      int cimg_siz, int cimg_w, uint32_t zimg, const int sc[4]);

#endif
