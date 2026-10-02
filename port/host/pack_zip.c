/*
 * A resource pack's files (pack.h): from a zip (stored or deflated members,
 * the deflate through stb_image's zlib decoder) or from a directory, the
 * zip's unpacked tree.  The pack's root is where pack.yaml is, so a zip of
 * the folder (pack/pack.yaml, ...) works as well as one of its contents.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pack.h"

#include "../third_party/stb/stb_image.h"

typedef struct {
    char *name;             /* below the root */
    uint32_t method, csize, usize, crc, local;
} zentry;

struct pack_files {
    char *dir;              /* a directory (with a trailing '/'), or NULL */
    uint8_t *zip;
    size_t zip_len;
    zentry *ents;
    int n;
};

static uint32_t rd16(const uint8_t *p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8; }
static uint32_t rd32(const uint8_t *p) { return rd16(p) | rd16(p + 2) << 16; }

static uint32_t crc32_of(const uint8_t *p, size_t n) {
    static uint32_t t[256];
    if (!t[1])
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t c = i;
            for (int k = 0; k < 8; k++)
                c = c & 1 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            t[i] = c;
        }
    uint32_t c = 0xFFFFFFFFu;
    while (n--)
        c = t[(c ^ *p++) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

static int ent_cmp(const void *a, const void *b) {
    return strcmp(((const zentry *)a)->name, ((const zentry *)b)->name);
}

static uint8_t *read_file(const char *path, size_t *len) {
    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *b = n >= 0 ? malloc((size_t)n + 1) : NULL;
    if (!b || fread(b, 1, (size_t)n, f) != (size_t)n) {
        free(b);
        fclose(f);
        return NULL;
    }
    fclose(f);
    b[n] = 0;
    *len = (size_t)n;
    return b;
}

/* a directory with a pack.yaml (stat() can fail in a 32-bit build, on a
   file system with 64-bit inode numbers) */
static int is_dir(const char *path) {
    size_t n = strlen(path);
    char *y = malloc(n + 11);
    memcpy(y, path, n);
    memcpy(y + n, "/pack.yaml", 11);
    FILE *f = fopen(y, "rb");
    free(y);
    if (f)
        fclose(f);
    return f != NULL;
}

pack_files *pack_open(const char *path, char *err, size_t errlen) {
    pack_files *f = calloc(1, sizeof *f);
    if (is_dir(path)) {
        size_t n = strlen(path);
        f->dir = malloc(n + 2);
        memcpy(f->dir, path, n);
        if (n == 0 || path[n - 1] != '/')
            f->dir[n++] = '/';
        f->dir[n] = 0;
        if (!pack_has(f, "pack.yaml")) {
            snprintf(err, errlen, "%s has no pack.yaml", path);
            pack_close(f);
            return NULL;
        }
        return f;
    }
    f->zip = read_file(path, &f->zip_len);
    if (!f->zip) {
        snprintf(err, errlen, "can't read %s", path);
        pack_close(f);
        return NULL;
    }
    /* the end of central directory record */
    const uint8_t *z = f->zip;
    size_t len = f->zip_len, eocd = (size_t)-1;
    for (size_t k = len >= 22 ? len - 22 : 0; len >= 22; k--) {
        if (rd32(z + k) == 0x06054B50) {
            eocd = k;
            break;
        }
        if (k == 0 || len - k > 22 + 65535)
            break;
    }
    if (eocd == (size_t)-1) {
        snprintf(err, errlen, "%s isn't a zip file", path);
        pack_close(f);
        return NULL;
    }
    uint32_t count = rd16(z + eocd + 10), cd = rd32(z + eocd + 16);
    if (count == 0xFFFF || cd == 0xFFFFFFFFu) {
        snprintf(err, errlen, "%s: zip64 isn't supported", path);
        pack_close(f);
        return NULL;
    }
    f->ents = calloc(count ? count : 1, sizeof *f->ents);
    size_t p = cd;
    const char *root = NULL;
    size_t root_len = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (p + 46 > len || rd32(z + p) != 0x02014B50) {
            snprintf(err, errlen, "%s: a broken central directory", path);
            pack_close(f);
            return NULL;
        }
        uint32_t flags = rd16(z + p + 8), nlen = rd16(z + p + 28), xlen = rd16(z + p + 30),
                 clen = rd16(z + p + 32);
        if (p + 46 + nlen > len)
            break;
        zentry *e = &f->ents[f->n];
        e->method = rd16(z + p + 10);
        e->crc = rd32(z + p + 16);
        e->csize = rd32(z + p + 20);
        e->usize = rd32(z + p + 24);
        e->local = rd32(z + p + 42);
        e->name = malloc(nlen + 1);
        memcpy(e->name, z + p + 46, nlen);
        e->name[nlen] = 0;
        for (char *c = e->name; *c; c++)
            if (*c == '\\')
                *c = '/';
        p += 46 + nlen + xlen + clen;
        if (nlen == 0 || e->name[nlen - 1] == '/') {     /* a directory */
            free(e->name);
            continue;
        }
        if (flags & 1) {
            snprintf(err, errlen, "%s: %s is encrypted", path, e->name);
            free(e->name);
            pack_close(f);
            return NULL;
        }
        /* the root: the shallowest pack.yaml */
        size_t nl = strlen(e->name);
        if ((nl == 9 || (nl > 9 && e->name[nl - 10] == '/')) && strcmp(e->name + nl - 9, "pack.yaml") == 0 &&
            (!root || nl - 9 < root_len)) {
            root = e->name;
            root_len = nl - 9;
        }
        f->n++;
    }
    if (!root) {
        snprintf(err, errlen, "%s has no pack.yaml", path);
        pack_close(f);
        return NULL;
    }
    /* below the root only, named from it */
    char *prefix = malloc(root_len + 1);
    memcpy(prefix, root, root_len);
    prefix[root_len] = 0;
    int m = 0;
    for (int i = 0; i < f->n; i++) {
        zentry e = f->ents[i];
        if (strncmp(e.name, prefix, root_len) != 0) {
            free(e.name);
            continue;
        }
        memmove(e.name, e.name + root_len, strlen(e.name) - root_len + 1);
        f->ents[m++] = e;
    }
    free(prefix);
    f->n = m;
    qsort(f->ents, (size_t)f->n, sizeof *f->ents, ent_cmp);
    return f;
}

static zentry *find(pack_files *f, const char *name) {
    zentry key = {.name = (char *)name};
    return f->ents ? bsearch(&key, f->ents, (size_t)f->n, sizeof *f->ents, ent_cmp) : NULL;
}

int pack_has(pack_files *f, const char *name) {
    if (f->dir) {
        char *path = malloc(strlen(f->dir) + strlen(name) + 1);
        strcpy(path, f->dir);
        strcat(path, name);
        FILE *h = fopen(path, "rb");
        free(path);
        if (h)
            fclose(h);
        return h != NULL;
    }
    return find(f, name) != NULL;
}

uint8_t *pack_read(pack_files *f, const char *name, size_t *len) {
    if (f->dir) {
        char *path = malloc(strlen(f->dir) + strlen(name) + 1);
        strcpy(path, f->dir);
        strcat(path, name);
        uint8_t *b = read_file(path, len);
        free(path);
        return b;
    }
    zentry *e = find(f, name);
    if (!e)
        return NULL;
    const uint8_t *z = f->zip;
    if ((size_t)e->local + 30 > f->zip_len || rd32(z + e->local) != 0x04034B50)
        return NULL;
    size_t data = e->local + 30 + rd16(z + e->local + 26) + rd16(z + e->local + 28);
    if (data + e->csize > f->zip_len)
        return NULL;
    uint8_t *out = malloc((size_t)e->usize + 1);
    if (e->method == 0) {
        if (e->csize != e->usize) {
            free(out);
            return NULL;
        }
        memcpy(out, z + data, e->usize);
    } else if (e->method == 8) {
        int got = stbi_zlib_decode_noheader_buffer((char *)out, (int)e->usize + 1, (const char *)z + data,
                                                   (int)e->csize);
        if (got != (int)e->usize) {
            free(out);
            return NULL;
        }
    } else {
        free(out);
        return NULL;
    }
    if (crc32_of(out, e->usize) != e->crc) {
        free(out);
        return NULL;
    }
    out[e->usize] = 0;
    *len = e->usize;
    return out;
}

void pack_close(pack_files *f) {
    if (!f)
        return;
    for (int i = 0; i < f->n; i++)
        free(f->ents[i].name);
    free(f->ents);
    free(f->zip);
    free(f->dir);
    free(f);
}
