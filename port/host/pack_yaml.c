/*
 * The YAML a resource pack is written in (pack.h): the block and flow
 * styles PyYAML writes and a text editor makes of them.
 *
 * Supported: block mappings and sequences (also nested on one line, "- - x"
 * and "- key: value"), a sequence as a mapping's value at the key's own
 * indentation, flow sequences and mappings (across lines too), plain,
 * single- and double-quoted scalars, literal and folded block scalars (|,
 * |-, |+, >), comments, a leading "---".  Not supported (an error, with the
 * line): anchors, aliases, tags, multi-line plain or quoted scalars,
 * several documents, tabs in indentation.  Scalars are kept as text; yint()
 * reads YAML 1.1's integers (PyYAML's: 0x.., 0o.. and 0.. octal, 0b..,
 * underscores), as the tools that write the pack do.
 */
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pack.h"

/* ---- allocation: one arena per document ---------------------------------- */

typedef struct yblock {
    struct yblock *next;
    size_t used, size;
    _Alignas(16) unsigned char data[];
} yblock;

typedef struct {
    yblock *head;
} yarena;

static void *aalloc(yarena *a, size_t n) {
    n = (n + 15) & ~(size_t)15;
    if (!a->head || a->head->used + n > a->head->size) {
        size_t sz = n > 65536 ? n : 65536;
        yblock *b = malloc(sizeof *b + sz);
        if (!b)
            abort();
        b->next = a->head;
        b->used = 0;
        b->size = sz;
        a->head = b;
    }
    void *p = a->head->data + a->head->used;
    a->head->used += n;
    return p;
}

static void afree(yarena *a) {
    for (yblock *b = a->head, *nx; b; b = nx) {
        nx = b->next;
        free(b);
    }
}

/* ---- the parser ----------------------------------------------------------- */

typedef struct {
    const char *s;
    int len;
    int indent;
} yline;

typedef struct {
    yline *lines;
    int nlines;
    int li, col;            /* the cursor */
    yarena *a;
    char *err;
    size_t errlen;
    int failed;
} P;

static void fail(P *p, int line, const char *fmt, ...) {
    if (p->failed)
        return;
    p->failed = 1;
    int k = snprintf(p->err, p->errlen, "line %d: ", line + 1);
    va_list ap;
    va_start(ap, fmt);
    if (k >= 0 && (size_t)k < p->errlen)
        vsnprintf(p->err + k, p->errlen - k, fmt, ap);
    va_end(ap);
}

static int eof(P *p) { return p->li >= p->nlines; }
static int ch(P *p) { return !eof(p) && p->col < p->lines[p->li].len ? (unsigned char)p->lines[p->li].s[p->col] : 0; }
static int ch_at(P *p, int col) {
    return !eof(p) && col < p->lines[p->li].len ? (unsigned char)p->lines[p->li].s[col] : 0;
}

static int line_blank(const yline *l) {
    int k = l->indent;
    while (k < l->len && (l->s[k] == ' ' || l->s[k] == '\t'))
        k++;
    return k == l->len || l->s[k] == '#';
}

/* to the next line with content; col at its indentation */
static void next_line(P *p) {
    p->li++;
    while (p->li < p->nlines && line_blank(&p->lines[p->li]))
        p->li++;
    if (!eof(p))
        p->col = p->lines[p->li].indent;
}

static void skip_sp(P *p) {
    while (ch(p) == ' ' || ch(p) == '\t')
        p->col++;
}

/* nothing but a comment left on the line */
static int at_eol(P *p) {
    skip_sp(p);
    return ch(p) == 0 || ch(p) == '#';
}

static ynode *node(P *p, int type) {
    ynode *n = aalloc(p->a, sizeof *n);
    memset(n, 0, sizeof *n);
    n->type = type;
    n->line = p->li;
    return n;
}

static char *dup_n(P *p, const char *s, int n) {
    char *d = aalloc(p->a, (size_t)n + 1);
    memcpy(d, s, (size_t)n);
    d[n] = 0;
    return d;
}

static ynode *scalar(P *p, const char *s, int n, int quoted) {
    ynode *y = node(p, Y_SCALAR);
    y->str = dup_n(p, s, n);
    y->quoted = quoted;
    return y;
}

/* growing item lists, copied into the arena when done */
typedef struct {
    ynode **items;
    char **keys;
    int n, cap;
} list;

static void list_add(list *l, char *key, ynode *v) {
    if (l->n == l->cap) {
        l->cap = l->cap ? l->cap * 2 : 8;
        l->items = realloc(l->items, sizeof *l->items * l->cap);
        l->keys = realloc(l->keys, sizeof *l->keys * l->cap);
    }
    l->keys[l->n] = key;
    l->items[l->n++] = v;
}

static void list_done(P *p, list *l, ynode *n) {
    n->n = l->n;
    n->items = aalloc(p->a, sizeof *n->items * (l->n + 1));
    memcpy(n->items, l->items, sizeof *n->items * l->n);
    if (n->type == Y_MAP) {
        n->keys = aalloc(p->a, sizeof *n->keys * (l->n + 1));
        memcpy(n->keys, l->keys, sizeof *n->keys * l->n);
    }
    free(l->items);
    free(l->keys);
}

/* a quoted scalar at the cursor (on one line) */
static ynode *quoted(P *p) {
    const yline *l = &p->lines[p->li];
    int q = ch(p), k = p->col + 1;
    char *buf = aalloc(p->a, (size_t)l->len + 1);
    int n = 0;
    for (;;) {
        if (k >= l->len) {
            fail(p, p->li, "a quoted string doesn't end on its line");
            return NULL;
        }
        char c = l->s[k++];
        if (q == '\'') {
            if (c == '\'') {
                if (k < l->len && l->s[k] == '\'') {
                    buf[n++] = '\'';
                    k++;
                    continue;
                }
                break;
            }
            buf[n++] = c;
        } else {
            if (c == '"')
                break;
            if (c != '\\') {
                buf[n++] = c;
                continue;
            }
            if (k >= l->len) {
                fail(p, p->li, "a backslash at the end of a quoted string");
                return NULL;
            }
            c = l->s[k++];
            switch (c) {
            case 'n': buf[n++] = '\n'; break;
            case 't': buf[n++] = '\t'; break;
            case 'r': buf[n++] = '\r'; break;
            case '0': buf[n++] = '\0'; break;
            case '\\': case '"': case '/': case ' ': buf[n++] = c; break;
            case 'x': {
                unsigned v = 0;
                for (int d = 0; d < 2; d++) {
                    if (k >= l->len || !isxdigit((unsigned char)l->s[k])) {
                        fail(p, p->li, "a bad \\x escape");
                        return NULL;
                    }
                    char h = l->s[k++];
                    v = v * 16 + (unsigned)(isdigit((unsigned char)h) ? h - '0' : (tolower(h) - 'a' + 10));
                }
                buf[n++] = (char)v;
                break;
            }
            default:
                fail(p, p->li, "the escape \\%c isn't supported", c);
                return NULL;
            }
        }
    }
    ynode *y = node(p, Y_SCALAR);
    buf[n] = 0;
    y->str = buf;
    y->quoted = 1;
    p->col = k;
    return y;
}

/* ---- flow style ------------------------------------------------------------ */

/* whitespace, line ends and comments between flow tokens */
static void flow_sp(P *p) {
    for (;;) {
        skip_sp(p);
        if (eof(p))
            return;
        if (ch(p) == 0 || ch(p) == '#') {
            next_line(p);
            continue;
        }
        return;
    }
}

static ynode *flow_node(P *p);

static ynode *flow_plain(P *p) {
    const yline *l = &p->lines[p->li];
    int k = p->col;
    while (k < l->len) {
        char c = l->s[k];
        if (c == ',' || c == '[' || c == ']' || c == '{' || c == '}')
            break;
        if (c == ':' && (k + 1 >= l->len || strchr(" ,]}", l->s[k + 1])))
            break;
        if (c == '#' && k > p->col && (l->s[k - 1] == ' ' || l->s[k - 1] == '\t'))
            break;
        k++;
    }
    int e = k;
    while (e > p->col && (l->s[e - 1] == ' ' || l->s[e - 1] == '\t'))
        e--;
    ynode *y = scalar(p, l->s + p->col, e - p->col, 0);
    p->col = k;
    return y;
}

static ynode *flow_scalar(P *p) {
    int c = ch(p);
    if (c == '\'' || c == '"')
        return quoted(p);
    if (c == '&' || c == '*' || c == '!') {
        fail(p, p->li, "anchors, aliases and tags aren't supported");
        return NULL;
    }
    return flow_plain(p);
}

static ynode *flow_coll(P *p) {
    int open = ch(p), close = open == '[' ? ']' : '}';
    int line = p->li;
    ynode *n = node(p, open == '[' ? Y_SEQ : Y_MAP);
    list l = {0};
    p->col++;
    for (;;) {
        flow_sp(p);
        if (eof(p)) {
            fail(p, line, "a '%c' that isn't closed", open);
            break;
        }
        if (ch(p) == close) {
            p->col++;
            break;
        }
        if (open == '[') {
            ynode *v = flow_node(p);
            if (!v)
                break;
            list_add(&l, NULL, v);
        } else {
            ynode *k = flow_scalar(p);
            if (!k)
                break;
            flow_sp(p);
            ynode *v;
            if (ch(p) == ':') {
                p->col++;
                flow_sp(p);
                if (ch(p) == ',' || ch(p) == '}')
                    v = node(p, Y_NULL);
                else if (!(v = flow_node(p)))
                    break;
            } else {
                v = node(p, Y_NULL);
            }
            list_add(&l, k->str, v);
        }
        flow_sp(p);
        if (ch(p) == ',') {
            p->col++;
            continue;
        }
        if (ch(p) != close) {
            fail(p, p->li, "expected ',' or '%c'", close);
            break;
        }
    }
    list_done(p, &l, n);
    return p->failed ? NULL : n;
}

static ynode *flow_node(P *p) {
    if (ch(p) == '[' || ch(p) == '{')
        return flow_coll(p);
    return flow_scalar(p);
}

/* ---- block style ----------------------------------------------------------- */

static ynode *value(P *p, int parent, int inline_ctx);

static int seq_dash(P *p, int col) {
    return ch_at(p, col) == '-' && (ch_at(p, col + 1) == 0 || ch_at(p, col + 1) == ' ' ||
                                    ch_at(p, col + 1) == '\t');
}

static ynode *block_seq(P *p, int scol) {
    ynode *n = node(p, Y_SEQ);
    list l = {0};
    for (;;) {
        ynode *v;
        p->col = scol + 1;
        if (at_eol(p)) {
            next_line(p);
            if (!eof(p) && p->lines[p->li].indent > scol)
                v = value(p, scol, 0);
            else
                v = node(p, Y_NULL);
        } else {
            v = value(p, scol, 0);
        }
        if (!v)
            break;
        list_add(&l, NULL, v);
        if (eof(p))
            break;
        int ind = p->lines[p->li].indent;
        if (ind == scol && seq_dash(p, scol))
            continue;
        if (ind > scol)
            fail(p, p->li, "this line is indented more than the sequence's items");
        break;
    }
    list_done(p, &l, n);
    return p->failed ? NULL : n;
}

/* the key at the cursor, the cursor after its ':' */
static char *map_key(P *p) {
    const yline *l = &p->lines[p->li];
    char *key;
    if (ch(p) == '\'' || ch(p) == '"') {
        ynode *k = quoted(p);
        if (!k)
            return NULL;
        skip_sp(p);
        key = k->str;
    } else {
        int k = p->col;
        while (k < l->len && !(l->s[k] == ':' && (k + 1 == l->len || l->s[k + 1] == ' ' || l->s[k + 1] == '\t')))
            k++;
        int e = k;
        while (e > p->col && (l->s[e - 1] == ' ' || l->s[e - 1] == '\t'))
            e--;
        key = dup_n(p, l->s + p->col, e - p->col);
        p->col = k;
    }
    if (ch(p) != ':') {
        fail(p, p->li, "expected 'key: value'");
        return NULL;
    }
    p->col++;
    return key;
}

static ynode *block_map(P *p, int mcol) {
    ynode *n = node(p, Y_MAP);
    list l = {0};
    for (;;) {
        p->col = mcol;
        char *key = map_key(p);
        if (!key)
            break;
        ynode *v;
        if (at_eol(p)) {
            next_line(p);
            if (eof(p))
                v = node(p, Y_NULL);
            else if (p->lines[p->li].indent > mcol)
                v = value(p, mcol, 0);
            else if (p->lines[p->li].indent == mcol && seq_dash(p, mcol))
                v = block_seq(p, mcol);
            else
                v = node(p, Y_NULL);
        } else {
            v = value(p, mcol, 1);
        }
        if (!v)
            break;
        for (int k = 0; k < l.n; k++)
            if (strcmp(l.keys[k], key) == 0) {
                fail(p, v->line, "the key '%s' twice", key);
                break;
            }
        list_add(&l, key, v);
        if (p->failed || eof(p))
            break;
        int ind = p->lines[p->li].indent;
        if (ind == mcol && !seq_dash(p, mcol))
            continue;
        if (ind > mcol)
            fail(p, p->li, "this line is indented more than the mapping's keys");
        break;
    }
    list_done(p, &l, n);
    return p->failed ? NULL : n;
}

/* | or > at the cursor; its lines are indented more than `parent` */
static ynode *block_scalar(P *p, int parent) {
    int line = p->li, fold = ch(p) == '>', chomp = 0, explicit = 0;
    p->col++;
    for (int k = 0; k < 2; k++) {
        if (ch(p) == '-' || ch(p) == '+') {
            chomp = ch(p) == '-' ? -1 : 1;
            p->col++;
        } else if (ch(p) >= '1' && ch(p) <= '9') {
            explicit = ch(p) - '0';
            p->col++;
        }
    }
    if (!at_eol(p)) {
        fail(p, line, "unexpected text after a block scalar's indicator");
        return NULL;
    }
    /* its lines: blank, or indented at least as much as the first */
    int li = line + 1, cind = explicit ? parent + explicit : -1;
    size_t cap = 64, n = 0;
    char *buf = malloc(cap);
    for (; li < p->nlines; li++) {
        const yline *l = &p->lines[li];
        int blank = 1;
        for (int k = 0; k < l->len; k++)
            if (l->s[k] != ' ') {
                blank = 0;
                break;
            }
        if (!blank) {
            if (cind < 0) {
                if (l->indent <= parent)
                    break;
                cind = l->indent;
            }
            if (l->indent < cind)
                break;
        }
        int from = blank ? (l->len < cind ? l->len : (cind < 0 ? l->len : cind)) : cind;
        size_t add = (size_t)(l->len - from) + 1;
        if (n + add + 1 > cap) {
            cap = (n + add + 1) * 2;
            buf = realloc(buf, cap);
        }
        memcpy(buf + n, l->s + from, (size_t)(l->len - from));
        n += (size_t)(l->len - from);
        buf[n++] = '\n';
    }
    if (chomp <= 0) {
        while (n > 0 && buf[n - 1] == '\n')
            n--;
        if (chomp == 0 && n > 0)
            buf[n++] = '\n';
    }
    if (fold) {
        for (size_t k = 0; k + 1 < n; k++)
            if (buf[k] == '\n' && buf[k + 1] != '\n' && (k == 0 || buf[k - 1] != '\n'))
                buf[k] = ' ';
    }
    ynode *y = scalar(p, buf, (int)n, 1);
    y->line = line;
    free(buf);
    p->li = li - 1;
    next_line(p);
    return y;
}

/* is there a "key:" at the cursor (outside quotes and flow)? */
static int looks_like_key(P *p) {
    const yline *l = &p->lines[p->li];
    int k = p->col;
    if (ch(p) == '\'' || ch(p) == '"') {
        P q = *p;
        char err[8];
        q.err = err;
        q.errlen = sizeof err;
        q.failed = 0;
        yarena a = {0};
        q.a = &a;
        ynode *s = quoted(&q);
        int ok = s && (skip_sp(&q), ch(&q) == ':') &&
                 (ch_at(&q, q.col + 1) == 0 || ch_at(&q, q.col + 1) == ' ' || ch_at(&q, q.col + 1) == '\t');
        afree(&a);
        return ok;
    }
    for (; k < l->len; k++) {
        if (l->s[k] == '#' && k > p->col && (l->s[k - 1] == ' ' || l->s[k - 1] == '\t'))
            return 0;
        if (l->s[k] == ':' && (k + 1 == l->len || l->s[k + 1] == ' ' || l->s[k + 1] == '\t'))
            return 1;
    }
    return 0;
}

/* the node at the cursor; afterwards the cursor is at the next line with
   content.  inline_ctx: right after "key:" on the same line */
static ynode *value(P *p, int parent, int inline_ctx) {
    int c = ch(p);
    if (c == '&' || c == '*' || c == '!') {
        fail(p, p->li, "anchors, aliases and tags aren't supported");
        return NULL;
    }
    if (seq_dash(p, p->col)) {
        if (inline_ctx) {
            fail(p, p->li, "a block sequence can't start on its key's line");
            return NULL;
        }
        return block_seq(p, p->col);
    }
    if (c == '|' || c == '>')
        return block_scalar(p, parent);
    if (!inline_ctx && c != '[' && c != '{' && looks_like_key(p))
        return block_map(p, p->col);
    ynode *y;
    int line = p->li;
    if (c == '[' || c == '{') {
        y = flow_coll(p);
    } else if (c == '\'' || c == '"') {
        y = quoted(p);
    } else {
        const yline *l = &p->lines[p->li];
        int k = p->col;
        while (k < l->len && !(l->s[k] == '#' && k > p->col && (l->s[k - 1] == ' ' || l->s[k - 1] == '\t')))
            k++;
        int e = k;
        while (e > p->col && (l->s[e - 1] == ' ' || l->s[e - 1] == '\t'))
            e--;
        y = scalar(p, l->s + p->col, e - p->col, 0);
        p->col = k;
    }
    if (!y)
        return NULL;
    y->line = line;
    if (!at_eol(p)) {
        fail(p, p->li, "unexpected text after a value");
        return NULL;
    }
    next_line(p);
    return y;
}

ynode *yaml_parse(const char *text, size_t len, char *err, size_t errlen) {
    yarena *a = calloc(1, sizeof *a);
    P p = {0};
    p.a = a;
    p.err = err;
    p.errlen = errlen;
    if (errlen)
        err[0] = 0;
    /* the lines */
    int cap = 1024;
    p.lines = malloc(sizeof *p.lines * cap);
    size_t k = 0;
    if (len >= 3 && memcmp(text, "\xEF\xBB\xBF", 3) == 0)
        k = 3;
    while (k < len) {
        size_t e = k;
        while (e < len && text[e] != '\n')
            e++;
        size_t le = e;
        if (le > k && text[le - 1] == '\r')
            le--;
        if (p.nlines == cap) {
            cap *= 2;
            p.lines = realloc(p.lines, sizeof *p.lines * cap);
        }
        yline *l = &p.lines[p.nlines++];
        l->s = text + k;
        l->len = (int)(le - k);
        l->indent = 0;
        while (l->indent < l->len && l->s[l->indent] == ' ')
            l->indent++;
        if (l->indent < l->len && l->s[l->indent] == '\t' && !line_blank(l)) {
            fail(&p, p.nlines - 1, "a tab in the indentation");
        }
        k = e + 1;
    }
    ynode *root = NULL;
    p.li = -1;
    next_line(&p);
    if (!p.failed && !eof(&p) && p.lines[p.li].len >= 3 && memcmp(p.lines[p.li].s, "---", 3) == 0 &&
        (p.lines[p.li].len == 3 || p.lines[p.li].s[3] == ' ')) {
        p.col = 3;
        if (at_eol(&p))
            next_line(&p);
    }
    if (!p.failed) {
        if (eof(&p)) {
            root = node(&p, Y_NULL);
        } else {
            root = value(&p, -1, 0);
            if (root && !eof(&p)) {
                if (p.lines[p.li].len >= 3 && memcmp(p.lines[p.li].s + p.lines[p.li].indent, "...", 3) == 0)
                    ;
                else
                    fail(&p, p.li, "unexpected text (a second document, or bad indentation)");
            }
        }
    }
    free(p.lines);
    if (p.failed || !root) {
        afree(a);
        free(a);
        return NULL;
    }
    root->arena = a;
    return root;
}

void yaml_free(ynode *n) {
    if (!n)
        return;
    yarena *a = n->arena;
    if (a) {
        afree(a);
        free(a);
    }
}

ynode *ymap(const ynode *m, const char *key) {
    if (!m || m->type != Y_MAP)
        return NULL;
    for (int k = 0; k < m->n; k++)
        if (strcmp(m->keys[k], key) == 0)
            return m->items[k];
    return NULL;
}

int ynull(const ynode *n) {
    if (!n || n->type == Y_NULL)
        return 1;
    if (n->type != Y_SCALAR || n->quoted)
        return 0;
    const char *s = n->str;
    return !*s || !strcmp(s, "~") || !strcmp(s, "null") || !strcmp(s, "Null") || !strcmp(s, "NULL");
}

int yint(const ynode *n, long long *out) {
    if (!n || n->type != Y_SCALAR || n->quoted)
        return 0;
    const char *s = n->str;
    int neg = 0;
    if (*s == '+' || *s == '-')
        neg = *s++ == '-';
    if (!*s)
        return 0;
    int base = 10;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        base = 16, s += 2;
    else if (s[0] == '0' && (s[1] == 'o' || s[1] == 'O'))
        base = 8, s += 2;
    else if (s[0] == '0' && (s[1] == 'b' || s[1] == 'B'))
        base = 2, s += 2;
    else if (s[0] == '0' && s[1])
        base = 8, s += 1;     /* YAML 1.1: a leading 0 is octal */
    if (!*s)
        return 0;
    unsigned long long v = 0;
    for (; *s; s++) {
        int d;
        if (*s == '_')
            continue;
        if (isdigit((unsigned char)*s))
            d = *s - '0';
        else if (isalpha((unsigned char)*s))
            d = tolower((unsigned char)*s) - 'a' + 10;
        else
            return 0;
        if (d >= base)
            return 0;
        v = v * (unsigned)base + (unsigned)d;
        if (v > (1ull << 62))
            return 0;
    }
    *out = neg ? -(long long)v : (long long)v;
    return 1;
}
