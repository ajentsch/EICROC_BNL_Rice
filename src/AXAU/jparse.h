/*
 * jparse.h - minimal, dependency-free JSON parser (C99, Linux/Windows).
 *
 * Builds a small DOM tree. Strings are returned as UTF-8.
 * Used internally by regjson, but usable on its own.
 */
#ifndef JPARSE_H
#define JPARSE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    JP_NULL, JP_BOOL, JP_NUMBER, JP_STRING, JP_ARRAY, JP_OBJECT
} jp_type;

typedef struct jp_value jp_value;
struct jp_value {
    jp_type   type;
    char     *key;        /* member name if inside an object, else NULL */
    union {
        int     boolean;
        double  number;
        char   *string;
    } u;
    jp_value *child;      /* first element / member (arrays, objects) */
    jp_value *next;       /* next sibling */
};

/* Parse a NUL-terminated JSON text. Returns NULL on error; if errbuf is
 * given it receives a message with line/column. Free with jp_free(). */
jp_value *jp_parse(const char *text, char *errbuf, size_t errlen);

void jp_free(jp_value *v);

/* Object member lookup (first match), NULL if missing / not an object. */
const jp_value *jp_get(const jp_value *obj, const char *key);

/* Number of elements / members of an array / object. */
size_t jp_count(const jp_value *v);

#ifdef __cplusplus
}
#endif
#endif /* JPARSE_H */
