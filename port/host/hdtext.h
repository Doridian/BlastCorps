/* --hd-text (hdtext.c, docs/FONTS.md): the game's text from a font, at
   high resolution, in the OpenGL renderer */
#ifndef HDTEXT_H
#define HDTEXT_H

#include <stdint.h>

#define HDTEXT_K 8                      /* a glyph's 32 texels become 32 * HDTEXT_K pixels */

extern int hdtext_on;                   /* --hd-text / PORT_HD_TEXT */
extern const char *hdtext_font_path;    /* a TrueType/OpenType file, or NULL: the built-in one */
extern float hdtext_weight;             /* PORT_HD_TEXT_WEIGHT: the ink area against the game's (1) */

/* the font, loaded after the options */
void hdtext_init(void);
/* at every retrace: makes the font's distance fields ahead, one at a time */
void hdtext_idle(void);
/* the texture table index of the glyph whose texture is at addr (an N64
   address), or -1 */
int hdtext_glyph_at(uint32_t addr);
/* the high-resolution coverage (32 * HDTEXT_K square, rows as the
   texture's) for glyph tex, whose game texture is rgba (32x32, as the
   renderer decoded it); NULL: draw the game's */
const uint8_t *hdtext_image(int tex, const uint8_t *rgba);
/* how many times glyph tex's image has been made: the renderer keeps one
   texture a glyph while this stays the same */
unsigned hdtext_serial(int tex);

#endif
