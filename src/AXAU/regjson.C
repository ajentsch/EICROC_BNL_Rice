/*
 * regjson.c - read chip register maps (EICROC-style JSON) in C.
 */
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "regjson.h"
#include "jparse.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define RJ_MAX_REGS   65536
#define RJ_MAX_PARTS  32

/* ------------------------------------------------------------ helpers */

static void set_err(char *err, size_t errlen, const char *fmt, ...)
{
    va_list ap;
    if (!err || !errlen) return;
    va_start(ap, fmt);
    vsnprintf(err, errlen, fmt, ap);
    va_end(ap);
}

static char *xstrndup(const char *s, size_t n)
{
    char *d = (char *)malloc(n + 1);
    if (d) { memcpy(d, s, n); d[n] = '\0'; }
    return d;
}

static char *xstrdup(const char *s) { return xstrndup(s, strlen(s)); }

static unsigned mask_bits(int w)
{
    return w >= 32 ? 0xFFFFFFFFu : ((1u << w) - 1u);
}

static int fwidth(const rj_field *f) { return f->msb - f->lsb + 1; }

static char *read_file(const char *path, char *err, size_t errlen)
{
    FILE *fp = fopen(path, "rb");
    long len;
    char *buf;

    if (!fp) { set_err(err, errlen, "cannot open '%s'", path); return NULL; }
    if (fseek(fp, 0, SEEK_END) != 0 || (len = ftell(fp)) < 0 ||
        fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        set_err(err, errlen, "cannot read '%s'", path);
        return NULL;
    }
    buf = (char *)malloc((size_t)len + 1);
    if (!buf) { fclose(fp); set_err(err, errlen, "out of memory"); return NULL; }
    if (fread(buf, 1, (size_t)len, fp) != (size_t)len) {
        free(buf); fclose(fp);
        set_err(err, errlen, "cannot read '%s'", path);
        return NULL;
    }
    buf[len] = '\0';
    fclose(fp);
    return buf;
}

/* "globalRegister_12" -> 12, -1 if no numeric suffix */
static int index_from_key(const char *key)
{
    const char *u = strrchr(key, '_'), *q;
    long n = 0;
    if (!u || !u[1]) return -1;
    for (q = u + 1; *q; q++) {
        if (*q < '0' || *q > '9') return -1;
        n = n * 10 + (*q - '0');
        if (n >= RJ_MAX_REGS) return -1;
    }
    return (int)n;
}

/* Split "dac_biaspa[5:2]" into base "dac_biaspa" and low index 2. */
static int split_name(rj_field *f)
{
    size_t len = strlen(f->name);
    const char *lb = strrchr(f->name, '[');
    int a, b;
    char tail;

    f->name_lo = -1;
    if (len >= 3 && f->name[len - 1] == ']' && lb && lb != f->name) {
        if (sscanf(lb, "[%d:%d%c", &a, &b, &tail) == 3 && tail == ']' &&
            a >= 0 && b >= 0)
            f->name_lo = a < b ? a : b;
        else if (sscanf(lb, "[%d%c", &a, &tail) == 2 && tail == ']' && a >= 0)
            f->name_lo = a;
    }
    f->base = f->name_lo >= 0 ? xstrndup(f->name, (size_t)(lb - f->name))
                              : xstrdup(f->name);
    return f->base != NULL;
}

static int get_int(const jp_value *obj, const char *key, int *out)
{
    const jp_value *v = jp_get(obj, key);
    if (!v || v->type != JP_NUMBER) return 0;
    if (!(v->u.number >= -1e9 && v->u.number <= 1e9)) return 0;  /* also NaN */
    if (v->u.number != (double)(int)v->u.number) return 0;
    *out = (int)v->u.number;
    return 1;
}

static char *get_str(const jp_value *obj, const char *key)
{
    const jp_value *v = jp_get(obj, key);
    return xstrdup(v && v->type == JP_STRING ? v->u.string : "");
}

static void free_register(rj_register *r)
{
    int i;
    for (i = 0; i < r->nfields; i++) {
        rj_field *f = &r->fields[i];
        free(f->name); free(f->base); free(f->label);
        free(f->tag);  free(f->description);
    }
    free(r->fields);
    free(r->key);
    memset(r, 0, sizeof *r);
}

/* ------------------------------------------------------------ loading */

static int load_field(rj_field *f, const jp_value *o, int width,
                      unsigned *used, const char *rkey,
                      char *err, size_t errlen)
{
    const jp_value *nm = jp_get(o, "name"), *dv = jp_get(o, "defaultValue"),
                   *enc = jp_get(o, "encoding");
    int nbits, i;

    if (!nm || nm->type != JP_STRING || !nm->u.string[0]) {
        set_err(err, errlen, "%s: field without \"name\"", rkey);
        return 0;
    }
    f->name        = xstrdup(nm->u.string);
    f->label       = get_str(o, "label");
    f->tag         = get_str(o, "tag");
    f->description = get_str(o, "description");
    if (!f->name || !f->label || !f->tag || !f->description || !split_name(f)) {
        set_err(err, errlen, "out of memory");
        return 0;
    }
    if (!get_int(o, "tagOrder", &f->tag_order)) f->tag_order = -1;
    f->inverted = enc && enc->type == JP_STRING &&
                  strcmp(enc->u.string, "inverted") == 0;

    if (get_int(o, "bitPosition", &f->msb)) {
        f->lsb = f->msb;
    } else if (!get_int(o, "bitPosition_MSB", &f->msb) ||
               !get_int(o, "bitPosition_LSB", &f->lsb)) {
        set_err(err, errlen, "%s/%s: missing bitPosition or "
                "bitPosition_MSB/LSB", rkey, f->name);
        return 0;
    }
    if (f->lsb < 0 || f->msb < f->lsb || f->msb >= width) {
        set_err(err, errlen, "%s/%s: bit range [%d:%d] invalid for width %d",
                rkey, f->name, f->msb, f->lsb, width);
        return 0;
    }
    nbits = fwidth(f);
    if (*used & (mask_bits(nbits) << f->lsb)) {
        set_err(err, errlen, "%s/%s: bits [%d:%d] overlap another field",
                rkey, f->name, f->msb, f->lsb);
        return 0;
    }
    *used |= mask_bits(nbits) << f->lsb;

    /* defaultValue: binary string, MSB first (a plain number is accepted too) */
    f->default_value = 0;
    if (dv && dv->type == JP_STRING) {
        const char *s = dv->u.string;
        if ((int)strlen(s) != nbits) {
            set_err(err, errlen, "%s/%s: defaultValue \"%s\" has %d bits, "
                    "field has %d", rkey, f->name, s, (int)strlen(s), nbits);
            return 0;
        }
        for (i = 0; i < nbits; i++) {
            if (s[i] != '0' && s[i] != '1') {
                set_err(err, errlen, "%s/%s: defaultValue \"%s\" is not binary",
                        rkey, f->name, s);
                return 0;
            }
            f->default_value = (f->default_value << 1) | (unsigned)(s[i] - '0');
        }
    } else if (dv && dv->type == JP_NUMBER && dv->u.number >= 0 &&
               dv->u.number < 2147483648.0 &&
               dv->u.number == (double)(unsigned)dv->u.number &&
               ((unsigned)dv->u.number & ~mask_bits(nbits)) == 0) {
        f->default_value = (unsigned)dv->u.number;
    } else {
        set_err(err, errlen, "%s/%s: missing or invalid defaultValue",
                rkey, f->name);
        return 0;
    }
    f->value = f->default_value;
    return 1;
}

static int load_register(rj_register *r, const char *key, const jp_value *obj,
                         char *err, size_t errlen)
{
    const jp_value *fl = jp_get(obj, "fields"), *it;
    unsigned used = 0;
    int i = 0;

    r->present = 1;
    r->key = xstrdup(key);
    if (!r->key) { set_err(err, errlen, "out of memory"); return 0; }
    if (!get_int(obj, "width", &r->width) || r->width < 1 || r->width > 31) {
        set_err(err, errlen, "%s: \"width\" missing or not in 1..31", key);
        return 0;
    }
    if (!fl || fl->type != JP_ARRAY) {
        set_err(err, errlen, "%s: \"fields\" missing or not an array", key);
        return 0;
    }
    r->nfields = (int)jp_count(fl);
    r->fields = (rj_field *)calloc(r->nfields ? (size_t)r->nfields : 1,
                                   sizeof(rj_field));
    if (!r->fields) { r->nfields = 0; set_err(err, errlen, "out of memory"); return 0; }

    for (it = fl->child; it; it = it->next, i++) {
        if (it->type != JP_OBJECT) {
            set_err(err, errlen, "%s: field #%d is not an object", key, i);
            return 0;
        }
        if (!load_field(&r->fields[i], it, r->width, &used, key, err, errlen))
            return 0;
    }
    return 1;
}

rj_map *rj_load_string(const char *json_text, char *err, size_t errlen)
{
    jp_value *root;
    const jp_value *it;
    rj_map *m = NULL;
    int maxidx = -1;

    if (err && errlen) err[0] = '\0';
    root = jp_parse(json_text, err, errlen);
    if (!root) return NULL;
    if (root->type != JP_OBJECT) {
        set_err(err, errlen, "top level JSON value is not an object");
        goto fail;
    }
    for (it = root->child; it; it = it->next) {
        int idx = index_from_key(it->key);
        if (idx >= 0 && it->type == JP_OBJECT && idx > maxidx) maxidx = idx;
    }
    if (maxidx < 0) {
        set_err(err, errlen, "no registers found (keys must end in _<number>)");
        goto fail;
    }
    m = (rj_map *)calloc(1, sizeof *m);
    if (!m) { set_err(err, errlen, "out of memory"); goto fail; }
    m->regs = (rj_register *)calloc((size_t)maxidx + 1, sizeof(rj_register));
    if (!m->regs) { set_err(err, errlen, "out of memory"); goto fail; }
    m->nregs = maxidx + 1;

    for (it = root->child; it; it = it->next) {
        int idx = index_from_key(it->key);
        if (idx < 0 || it->type != JP_OBJECT) continue;   /* e.g. metadata */
        if (m->regs[idx].present) {
            set_err(err, errlen, "%s: register %d defined twice", it->key, idx);
            goto fail;
        }
        if (!load_register(&m->regs[idx], it->key, it, err, errlen))
            goto fail;
    }
    jp_free(root);
    return m;
fail:
    rj_free(m);
    jp_free(root);
    return NULL;
}

rj_map *rj_load(const char *path, char *err, size_t errlen)
{
    char *text = read_file(path, err, errlen);
    rj_map *m;
    if (!text) return NULL;
    m = rj_load_string(text, err, errlen);
    free(text);
    return m;
}

void rj_free(rj_map *m)
{
    int i;
    if (!m) return;
    for (i = 0; i < m->nregs; i++) free_register(&m->regs[i]);
    free(m->regs);
    free(m);
}

/* ------------------------------------------------------ register access */

const rj_register *rj_register_info(const rj_map *m, int reg)
{
    if (!m || reg < 0 || reg >= m->nregs || !m->regs[reg].present) return NULL;
    return &m->regs[reg];
}

static int assemble(const rj_map *m, int reg, int use_default)
{
    const rj_register *r = rj_register_info(m, reg);
    unsigned v = 0;
    int i;
    if (!r) return RJ_ERR;
    for (i = 0; i < r->nfields; i++) {
        const rj_field *f = &r->fields[i];
        unsigned fv = use_default ? f->default_value : f->value;
        v |= (fv & mask_bits(fwidth(f))) << f->lsb;
    }
    return (int)v;
}

int rj_read(const rj_map *m, int reg)         { return assemble(m, reg, 0); }
int rj_read_default(const rj_map *m, int reg) { return assemble(m, reg, 1); }

int rj_write(rj_map *m, int reg, unsigned value)
{
    rj_register *r;
    int i;
    if (!rj_register_info(m, reg)) return RJ_ERR;
    r = &m->regs[reg];
    if (value & ~mask_bits(r->width)) return RJ_RANGE;
    for (i = 0; i < r->nfields; i++) {
        rj_field *f = &r->fields[i];
        f->value = (value >> f->lsb) & mask_bits(fwidth(f));
    }
    return RJ_OK;
}

void rj_reset(rj_map *m)
{
    int i, j;
    if (!m) return;
    for (i = 0; i < m->nregs; i++)
        for (j = 0; j < m->regs[i].nfields; j++)
            m->regs[i].fields[j].value = m->regs[i].fields[j].default_value;
}

int rj_read_all(const rj_map *m, unsigned *out, int max)
{
    int i, n;
    if (!m || !out || max <= 0) return 0;
    n = m->nregs < max ? m->nregs : max;
    for (i = 0; i < n; i++) {
        int v = rj_read(m, i);
        out[i] = v < 0 ? 0u : (unsigned)v;
    }
    return n;
}

/* --------------------------------------------------------- field access */

typedef struct { rj_field *f; int offset; } rj_part;

/* Collect the parts of a logical field and their bit offsets.
 * Returns number of parts, or RJ_ERR / RJ_AMBIGUOUS. */
static int collect(const rj_map *m, const char *name, rj_part *p, int *nbits)
{
    int pass, n = 0, i, j, all_br = 1, all_to = 1, minlo = 1 << 30, width = 0;
    unsigned lmask = 0;

    if (!m || !name || !*name) return RJ_ERR;
    for (pass = 0; pass < 3 && n == 0; pass++) {
        for (i = 0; i < m->nregs; i++) {
            rj_register *r = &m->regs[i];
            for (j = 0; j < r->nfields; j++) {
                rj_field *f = &r->fields[j];
                const char *s = pass == 0 ? f->name : pass == 1 ? f->base : f->tag;
                if (s[0] && strcmp(s, name) == 0) {
                    if (n >= RJ_MAX_PARTS) return RJ_AMBIGUOUS;
                    p[n++].f = f;
                }
            }
        }
    }
    if (n == 0) return RJ_ERR;

    for (i = 0; i < n; i++) {
        if (p[i].f->name_lo < 0) all_br = 0;
        else if (p[i].f->name_lo < minlo) minlo = p[i].f->name_lo;
        if (p[i].f->tag_order < 0) all_to = 0;
    }
    if (n == 1) {
        p[0].offset = 0;
    } else if (all_br) {
        for (i = 0; i < n; i++) p[i].offset = p[i].f->name_lo - minlo;
    } else if (all_to) {
        int off = 0;
        for (i = 1; i < n; i++) {                 /* insertion sort by tagOrder */
            rj_part t = p[i];
            for (j = i; j > 0 && p[j - 1].f->tag_order > t.f->tag_order; j--)
                p[j] = p[j - 1];
            p[j] = t;
        }
        for (i = 0; i < n; i++) { p[i].offset = off; off += fwidth(p[i].f); }
    } else {
        return RJ_AMBIGUOUS;
    }

    for (i = 0; i < n; i++) {
        int w = fwidth(p[i].f);
        unsigned pm;
        if (p[i].offset + w > 31) return RJ_AMBIGUOUS;  /* fits in long */
        pm = mask_bits(w) << p[i].offset;
        if (lmask & pm) return RJ_AMBIGUOUS;          /* overlapping parts */
        lmask |= pm;
        if (p[i].offset + w > width) width = p[i].offset + w;
    }
    if (nbits) *nbits = width;
    return n;
}

static int get_field(const rj_map *m, const char *name, unsigned *value,
                     int *nbits, int use_default)
{
    rj_part p[RJ_MAX_PARTS];
    unsigned v = 0;
    int i, n = collect(m, name, p, nbits);
    if (n < 0) return n;
    for (i = 0; i < n; i++)
        v |= (use_default ? p[i].f->default_value : p[i].f->value) << p[i].offset;
    if (value) *value = v;
    return RJ_OK;
}

int rj_get_field(const rj_map *m, const char *name, unsigned *value, int *nbits)
{
    return get_field(m, name, value, nbits, 0);
}

int rj_get_field_default(const rj_map *m, const char *name, unsigned *value,
                         int *nbits)
{
    return get_field(m, name, value, nbits, 1);
}

int rj_set_field(rj_map *m, const char *name, unsigned value)
{
    rj_part p[RJ_MAX_PARTS];
    int i, nbits = 0, n = collect(m, name, p, &nbits);
    if (n < 0) return n;
    if (value & ~mask_bits(nbits)) return RJ_RANGE;
    for (i = 0; i < n; i++)
        p[i].f->value = (value >> p[i].offset) & mask_bits(fwidth(p[i].f));
    return RJ_OK;
}

/* ----------------------------------------------------------------- dump */

static void print_bin(FILE *out, unsigned v, int nbits)
{
    int i;
    for (i = nbits - 1; i >= 0; i--) fputc((v >> i) & 1u ? '1' : '0', out);
}

void rj_dump(const rj_map *m, FILE *out)
{
    int i, j;
    if (!m || !out) return;
    for (i = 0; i < m->nregs; i++) {
        const rj_register *r = rj_register_info(m, i);
        int v, d;
        if (!r) continue;
        v = rj_read(m, i);
        d = rj_read_default(m, i);
        fprintf(out, "Reg %3d  %-20s = 0x%0*X  ", i, r->key, (r->width + 3) / 4, v);
        print_bin(out, (unsigned)v, r->width);
        if (v != d) fprintf(out, "   (default 0x%0*X)", (r->width + 3) / 4, d);
        fputc('\n', out);
        for (j = 0; j < r->nfields; j++) {
            const rj_field *f = &r->fields[j];
            char bits[16];
            if (f->msb == f->lsb) snprintf(bits, sizeof bits, "[%d]", f->msb);
            else snprintf(bits, sizeof bits, "[%d:%d]", f->msb, f->lsb);
            fprintf(out, "    %-7s %-34s ", bits, f->name);
            print_bin(out, f->value, fwidth(f));
            fprintf(out, "%*s", 10 - fwidth(f) > 0 ? 10 - fwidth(f) : 1, "");
            if (f->value != f->default_value || f->inverted)
                fprintf(out, "%-14s%s%s\n", f->label,
                        f->value != f->default_value ? "*changed* " : "",
                        f->inverted ? "(inverted)" : "");
            else
                fprintf(out, "%s\n", f->label);
        }
    }
}

/* ------------------------------------------- descriptor-based json_* API */

typedef struct {
    rj_map *map;
    char   *path;
} rj_slot;

static rj_slot g_files[JSON_MAX_FILES];
static char    g_err[512] = "";

static rj_map *desc_map(int desc)
{
    if (desc < 0 || desc >= JSON_MAX_FILES || !g_files[desc].map) {
        snprintf(g_err, sizeof g_err, "invalid descriptor %d", desc);
        return NULL;
    }
    return g_files[desc].map;
}

int json_load(const char *path)
{
    int d;
    rj_map *m;
    char *p;

    if (!path) { snprintf(g_err, sizeof g_err, "NULL path"); return RJ_ERR; }
    for (d = 0; d < JSON_MAX_FILES && g_files[d].map; d++)
        ;
    if (d == JSON_MAX_FILES) {
        snprintf(g_err, sizeof g_err, "too many open files (max %d)", JSON_MAX_FILES);
        return RJ_ERR;
    }
    m = rj_load(path, g_err, sizeof g_err);
    if (!m) return RJ_ERR;
    p = xstrdup(path);
    if (!p) { rj_free(m); snprintf(g_err, sizeof g_err, "out of memory"); return RJ_ERR; }
    g_files[d].map = m;
    g_files[d].path = p;
    g_err[0] = '\0';
    return d;
}

int json_close(int desc)
{
    if (!desc_map(desc)) return RJ_ERR;
    rj_free(g_files[desc].map);
    free(g_files[desc].path);
    g_files[desc].map = NULL;
    g_files[desc].path = NULL;
    return RJ_OK;
}

void json_close_all(void)
{
    int d;
    for (d = 0; d < JSON_MAX_FILES; d++)
        if (g_files[d].map) json_close(d);
}

const char *json_error(void)        { return g_err; }
rj_map *json_map(int desc)          { return desc_map(desc); }

const char *json_filename(int desc)
{
    return desc_map(desc) ? g_files[desc].path : NULL;
}

int json_num_registers(int desc)
{
    rj_map *m = desc_map(desc);
    return m ? m->nregs : RJ_ERR;
}

int json_reset(int desc)
{
    rj_map *m = desc_map(desc);
    if (!m) return RJ_ERR;
    rj_reset(m);
    return RJ_OK;
}

int json_dump(int desc, FILE *out)
{
    rj_map *m = desc_map(desc);
    if (!m) return RJ_ERR;
    rj_dump(m, out);
    return RJ_OK;
}

int json_read_all(int desc, unsigned *out, int max)
{
    rj_map *m = desc_map(desc);
    return m ? rj_read_all(m, out, max) : RJ_ERR;
}

int json_read(int desc, int reg)
{
    rj_map *m = desc_map(desc);
    int v;
    if (!m) return RJ_ERR;
    v = rj_read(m, reg);
    if (v < 0) snprintf(g_err, sizeof g_err, "%s: register %d not defined",
                        g_files[desc].path, reg);
    return v;
}

int json_read_default(int desc, int reg)
{
    rj_map *m = desc_map(desc);
    int v;
    if (!m) return RJ_ERR;
    v = rj_read_default(m, reg);
    if (v < 0) snprintf(g_err, sizeof g_err, "%s: register %d not defined",
                        g_files[desc].path, reg);
    return v;
}

int json_write(int desc, int reg, unsigned value)
{
    rj_map *m = desc_map(desc);
    int rc;
    if (!m) return RJ_ERR;
    rc = rj_write(m, reg, value);
    if (rc == RJ_ERR)
        snprintf(g_err, sizeof g_err, "%s: register %d not defined",
                 g_files[desc].path, reg);
    else if (rc == RJ_RANGE)
        snprintf(g_err, sizeof g_err, "%s: value 0x%X too wide for register %d",
                 g_files[desc].path, value, reg);
    return rc;
}

static void field_err(int desc, int rc, const char *name)
{
    const char *f = g_files[desc].path;
    if (rc == RJ_ERR)
        snprintf(g_err, sizeof g_err, "%s: field '%s' not found", f, name);
    else if (rc == RJ_AMBIGUOUS)
        snprintf(g_err, sizeof g_err, "%s: field '%s' is ambiguous", f, name);
    else if (rc == RJ_RANGE)
        snprintf(g_err, sizeof g_err, "%s: value too wide for field '%s'", f, name);
}

long json_get(int desc, const char *name)
{
    rj_map *m = desc_map(desc);
    unsigned v;
    int rc;
    if (!m) return RJ_ERR;
    rc = rj_get_field(m, name, &v, NULL);
    if (rc < 0) { field_err(desc, rc, name ? name : "(null)"); return rc; }
    return (long)v;
}

int json_set(int desc, const char *name, unsigned value)
{
    rj_map *m = desc_map(desc);
    int rc;
    if (!m) return RJ_ERR;
    rc = rj_set_field(m, name, value);
    if (rc < 0) field_err(desc, rc, name ? name : "(null)");
    return rc;
}
