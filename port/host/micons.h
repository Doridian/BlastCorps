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

#endif
