/*
 * Resource packs (docs/PORT.md, "Resource packs"; the format in
 * docs/ASSETS.md, "The pack"): a zip of the ROM's assets as editable files,
 * which the port rebuilds the ROM's image from at startup (pack.c), with
 * the zip reader (pack_zip.c) and the YAML it reads (pack_yaml.c).
 */
#ifndef PORT_PACK_H
#define PORT_PACK_H

#include <stddef.h>
#include <stdint.h>

/* ---- pack.c ---------------------------------------------------------------- */

int pack_is_pack(const char *path);         /* a zip, or a directory with pack.yaml */
/* the ROM image the pack makes, as the port's link expects it (malloc'd) */
uint8_t *pack_build_rom(const char *path, uint32_t *size, int *edited);
/* the code modules' data at their physical addresses (RDRAM-sized), from the
   pack's data/, or NULL: port/host/romdata.c's source */
const uint8_t *pack_data_source(void);
/* an edited texture's texels (len bytes, as the game decodes it), or NULL */
const uint8_t *host_tex_texels(uint32_t id, uint32_t *len);
/* an edited texture's higher-resolution image, for the renderer (gfx_gl.c) */
const uint8_t *host_tex_hires(uint32_t addr, int w, int h, int *k, int *id);

/* ---- pack_zip.c ------------------------------------------------------------ */

typedef struct pack_files pack_files;
pack_files *pack_open(const char *path, char *err, size_t errlen);   /* zip or directory */
/* a file's contents (malloc'd, NUL-terminated past *len), or NULL if absent */
uint8_t *pack_read(pack_files *f, const char *name, size_t *len);
int pack_has(pack_files *f, const char *name);
void pack_close(pack_files *f);

/* ---- pack_yaml.c ----------------------------------------------------------- */

enum { Y_NULL, Y_SCALAR, Y_SEQ, Y_MAP };
typedef struct ynode ynode;
struct ynode {
    int type;
    int line;
    int quoted;         /* a scalar written in quotes (never a number or null) */
    char *str;          /* Y_SCALAR */
    int n;              /* Y_SEQ, Y_MAP: items */
    ynode **items;
    char **keys;        /* Y_MAP */
    void *arena;        /* the root's: everything the document allocated */
};
/* the document in text[0..len); NULL with a message in err */
ynode *yaml_parse(const char *text, size_t len, char *err, size_t errlen);
void yaml_free(ynode *n);
ynode *ymap(const ynode *m, const char *key);   /* NULL if m isn't a map or lacks it */
int yint(const ynode *n, long long *out);       /* YAML 1.1's integers; 0 if it isn't one */
int ynull(const ynode *n);                      /* absent, or ~/null/empty */

#endif
