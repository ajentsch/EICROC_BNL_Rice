/*
 * jparse.c - minimal, dependency-free JSON parser (C99, Linux/Windows).
 */
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "jparse.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define JP_MAX_DEPTH 256

typedef struct {
    const char *start;
    const char *p;
    char       *err;
    size_t      errlen;
    int         depth;
} jp_ctx;

static jp_value *parse_value(jp_ctx *c);

static void jp_fail(jp_ctx *c, const char *msg)
{
    if (c->err && c->errlen && c->err[0] == '\0') {
        int line = 1, col = 1;
        const char *q;
        for (q = c->start; q < c->p && *q; q++) {
            if (*q == '\n') { line++; col = 1; } else col++;
        }
        snprintf(c->err, c->errlen, "JSON error at line %d, col %d: %s",
                 line, col, msg);
    }
}

static jp_value *jp_new(jp_type t)
{
    jp_value *v = (jp_value *)calloc(1, sizeof *v);
    if (v) v->type = t;
    return v;
}

static void skip_ws(jp_ctx *c)
{
    while (*c->p == ' ' || *c->p == '\t' || *c->p == '\n' || *c->p == '\r')
        c->p++;
}

static int is_digit(char ch) { return ch >= '0' && ch <= '9'; }

static int hex4(const char *s, unsigned *out)
{
    unsigned v = 0;
    int i;
    for (i = 0; i < 4; i++) {
        char ch = s[i];
        v <<= 4;
        if (ch >= '0' && ch <= '9')      v |= (unsigned)(ch - '0');
        else if (ch >= 'a' && ch <= 'f') v |= (unsigned)(ch - 'a' + 10);
        else if (ch >= 'A' && ch <= 'F') v |= (unsigned)(ch - 'A' + 10);
        else return 0;
    }
    *out = v;
    return 1;
}

static int utf8_encode(unsigned cp, char *o)
{
    if (cp < 0x80) { o[0] = (char)cp; return 1; }
    if (cp < 0x800) {
        o[0] = (char)(0xC0 | (cp >> 6));
        o[1] = (char)(0x80 | (cp & 0x3F));
        return 2;
    }
    if (cp < 0x10000) {
        o[0] = (char)(0xE0 | (cp >> 12));
        o[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        o[2] = (char)(0x80 | (cp & 0x3F));
        return 3;
    }
    o[0] = (char)(0xF0 | (cp >> 18));
    o[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
    o[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
    o[3] = (char)(0x80 | (cp & 0x3F));
    return 4;
}

/* c->p at opening quote. Returns malloc'd UTF-8 string or NULL. */
static char *parse_string_raw(jp_ctx *c)
{
    const char *q;
    char *out, *o;

    c->p++;
    for (q = c->p; *q && *q != '"'; q++)
        if (*q == '\\' && q[1]) q++;
    if (*q != '"') { jp_fail(c, "unterminated string"); return NULL; }

    /* escapes never expand, so the raw length is an upper bound */
    out = (char *)malloc((size_t)(q - c->p) + 1);
    if (!out) { jp_fail(c, "out of memory"); return NULL; }
    o = out;

    while (*c->p != '"') {
        unsigned char ch = (unsigned char)*c->p;
        if (ch < 0x20) { jp_fail(c, "control character in string"); goto bad; }
        if (ch != '\\') { *o++ = (char)ch; c->p++; continue; }
        c->p++;
        switch (*c->p) {
        case '"':  *o++ = '"';  break;
        case '\\': *o++ = '\\'; break;
        case '/':  *o++ = '/';  break;
        case 'b':  *o++ = '\b'; break;
        case 'f':  *o++ = '\f'; break;
        case 'n':  *o++ = '\n'; break;
        case 'r':  *o++ = '\r'; break;
        case 't':  *o++ = '\t'; break;
        case 'u': {
            unsigned cp, lo;
            if (!hex4(c->p + 1, &cp)) { jp_fail(c, "bad \\u escape"); goto bad; }
            c->p += 4;
            if (cp >= 0xD800 && cp <= 0xDBFF) {
                if (c->p[1] == '\\' && c->p[2] == 'u' && hex4(c->p + 3, &lo) &&
                    lo >= 0xDC00 && lo <= 0xDFFF) {
                    cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                    c->p += 6;
                } else { jp_fail(c, "invalid surrogate pair"); goto bad; }
            } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
                jp_fail(c, "invalid surrogate pair"); goto bad;
            }
            o += utf8_encode(cp, o);
            break;
        }
        default:
            jp_fail(c, "invalid escape sequence"); goto bad;
        }
        c->p++;
    }
    c->p++;            /* closing quote */
    *o = '\0';
    return out;
bad:
    free(out);
    return NULL;
}

/* Locale-independent number parsing (strtod depends on LC_NUMERIC). */
static jp_value *parse_number(jp_ctx *c)
{
    const char *q = c->p;
    double val = 0.0, scale = 1.0;
    int neg = 0, eneg = 0, ex = 0;
    jp_value *v;

    if (*q == '-') { neg = 1; q++; }
    if (*q == '0') q++;
    else if (is_digit(*q)) { while (is_digit(*q)) val = val * 10.0 + (*q++ - '0'); }
    else { c->p = q; jp_fail(c, "invalid number"); return NULL; }

    if (*q == '.') {
        q++;
        if (!is_digit(*q)) { c->p = q; jp_fail(c, "invalid number"); return NULL; }
        while (is_digit(*q)) { scale /= 10.0; val += (*q++ - '0') * scale; }
    }
    if (*q == 'e' || *q == 'E') {
        q++;
        if (*q == '+' || *q == '-') eneg = (*q++ == '-');
        if (!is_digit(*q)) { c->p = q; jp_fail(c, "invalid number"); return NULL; }
        while (is_digit(*q)) { if (ex < 10000) ex = ex * 10 + (*q - '0'); q++; }
        while (ex-- > 0) val = eneg ? val / 10.0 : val * 10.0;
    }
    v = jp_new(JP_NUMBER);
    if (!v) { jp_fail(c, "out of memory"); return NULL; }
    v->u.number = neg ? -val : val;
    c->p = q;
    return v;
}

static jp_value *parse_container(jp_ctx *c, int is_obj)
{
    jp_value *v = jp_new(is_obj ? JP_OBJECT : JP_ARRAY), *tail = NULL;
    char close = is_obj ? '}' : ']';

    if (!v) { jp_fail(c, "out of memory"); return NULL; }
    if (++c->depth > JP_MAX_DEPTH) { jp_fail(c, "nesting too deep"); goto bad; }
    c->p++;
    skip_ws(c);
    if (*c->p == close) { c->p++; c->depth--; return v; }

    for (;;) {
        char *key = NULL;
        jp_value *item;

        skip_ws(c);
        if (is_obj) {
            if (*c->p != '"') { jp_fail(c, "expected member name"); goto bad; }
            key = parse_string_raw(c);
            if (!key) goto bad;
            skip_ws(c);
            if (*c->p != ':') { free(key); jp_fail(c, "expected ':'"); goto bad; }
            c->p++;
        }
        item = parse_value(c);
        if (!item) { free(key); goto bad; }
        item->key = key;
        if (tail) tail->next = item; else v->child = item;
        tail = item;

        skip_ws(c);
        if (*c->p == ',') { c->p++; continue; }
        if (*c->p == close) { c->p++; break; }
        jp_fail(c, is_obj ? "expected ',' or '}'" : "expected ',' or ']'");
        goto bad;
    }
    c->depth--;
    return v;
bad:
    jp_free(v);
    return NULL;
}

static jp_value *parse_value(jp_ctx *c)
{
    jp_value *v;
    skip_ws(c);
    switch (*c->p) {
    case '{': return parse_container(c, 1);
    case '[': return parse_container(c, 0);
    case '"': {
        char *s = parse_string_raw(c);
        if (!s) return NULL;
        v = jp_new(JP_STRING);
        if (!v) { free(s); jp_fail(c, "out of memory"); return NULL; }
        v->u.string = s;
        return v;
    }
    case 't':
        if (strncmp(c->p, "true", 4) == 0) {
            c->p += 4; v = jp_new(JP_BOOL); if (v) v->u.boolean = 1; return v;
        }
        break;
    case 'f':
        if (strncmp(c->p, "false", 5) == 0) {
            c->p += 5; v = jp_new(JP_BOOL); return v;
        }
        break;
    case 'n':
        if (strncmp(c->p, "null", 4) == 0) {
            c->p += 4; return jp_new(JP_NULL);
        }
        break;
    case '\0':
        jp_fail(c, "unexpected end of input");
        return NULL;
    default:
        if (*c->p == '-' || is_digit(*c->p)) return parse_number(c);
        break;
    }
    jp_fail(c, "unexpected character");
    return NULL;
}

jp_value *jp_parse(const char *text, char *errbuf, size_t errlen)
{
    jp_ctx c;
    jp_value *root;

    if (errbuf && errlen) errbuf[0] = '\0';
    if (!text) return NULL;
    if ((unsigned char)text[0] == 0xEF && (unsigned char)text[1] == 0xBB &&
        (unsigned char)text[2] == 0xBF)
        text += 3;                         /* skip UTF-8 BOM */

    c.start = c.p = text;
    c.err = errbuf;
    c.errlen = errlen;
    c.depth = 0;

    root = parse_value(&c);
    if (!root) return NULL;
    skip_ws(&c);
    if (*c.p != '\0') {
        jp_fail(&c, "trailing characters after JSON value");
        jp_free(root);
        return NULL;
    }
    return root;
}

void jp_free(jp_value *v)
{
    while (v) {
        jp_value *n = v->next;
        jp_free(v->child);
        free(v->key);
        if (v->type == JP_STRING) free(v->u.string);
        free(v);
        v = n;
    }
}

const jp_value *jp_get(const jp_value *obj, const char *key)
{
    const jp_value *m;
    if (!obj || obj->type != JP_OBJECT || !key) return NULL;
    for (m = obj->child; m; m = m->next)
        if (m->key && strcmp(m->key, key) == 0) return m;
    return NULL;
}

size_t jp_count(const jp_value *v)
{
    size_t n = 0;
    const jp_value *m;
    if (!v || (v->type != JP_ARRAY && v->type != JP_OBJECT)) return 0;
    for (m = v->child; m; m = m->next) n++;
    return n;
}
