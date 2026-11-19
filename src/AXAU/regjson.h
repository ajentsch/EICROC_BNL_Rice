/*
 * regjson.h - read chip register maps (EICROC-style JSON) in C.
 *
 * JSON layout expected:
 *   { "<anything>_<N>": { "width": 8,
 *                         "fields": [ { "name": "...",
 *                                       "bitPosition": 7              (1 bit)
 *                                       | "bitPosition_MSB"/"_LSB",   (range)
 *                                       "defaultValue": "0110",       (binary, MSB first)
 *                                       "label", "tag", "tagOrder",
 *                                       "encoding", "description"  (optional) } ] },
 *     ... }
 *   <N> is the register number (e.g. "globalRegister_12" -> 12).
 *
 * Two APIs:
 *   1. json_*  : descriptor-based API, several files open at once
 *                d = json_load(path); json_read(d, reg); json_set(d, name, v) ...
 *   2. rj_*    : the same functionality on rj_map pointers (no file limit).
 *
 * Every field holds a "current" value, initialised from defaultValue.
 * Reads assemble register words from the current field values.
 * Register widths of 1..31 bits are supported.
 */
#ifndef REGJSON_H
#define REGJSON_H

#include <stdio.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------- types */

typedef struct {
    char    *name;         /* full name, e.g. "dac_biaspa[5:2]"              */
    char    *base;         /* name without trailing [..], e.g. "dac_biaspa"  */
    char    *label;        /* "" if absent                                   */
    char    *tag;          /* "" if absent                                   */
    char    *description;  /* "" if absent                                   */
    int      tag_order;    /* -1 if absent                                   */
    int      msb, lsb;     /* bit position(s) in the register                */
    int      name_lo;      /* low index of the [a:b]/[a] name suffix, or -1  */
    int      inverted;     /* 1 if "encoding": "inverted" (informational)    */
    unsigned default_value;/* right-aligned                                  */
    unsigned value;        /* current value, right-aligned                   */
} rj_field;

typedef struct {
    int       present;     /* 0 if this index is not defined in the file     */
    char     *key;         /* JSON key, e.g. "globalRegister_3"              */
    int       width;       /* register width in bits                         */
    int       nfields;
    rj_field *fields;
} rj_register;

typedef struct {
    int          nregs;    /* highest register index + 1                     */
    rj_register *regs;     /* indexed by register number                     */
} rj_map;

/* Error codes returned by the int functions */
#define RJ_OK          0
#define RJ_ERR        -1   /* bad register / name not found / not loaded     */
#define RJ_AMBIGUOUS  -2   /* field parts cannot be combined unambiguously   */
#define RJ_RANGE      -3   /* value does not fit in the register / field     */

/* ------------------------------------------- descriptor-based json_* API */
/*
 * json_load() returns a small integer descriptor (like open()) that selects
 * the file in all other json_* calls. Up to JSON_MAX_FILES files can be open
 * at once; closed descriptors are reused.
 */
#ifndef JSON_MAX_FILES
#define JSON_MAX_FILES 16
#endif

int         json_load(const char *path);      /* descriptor >= 0, or -1 (see json_error) */
int         json_close(int desc);             /* 0 ok, -1 bad descriptor      */
void        json_close_all(void);
const char *json_error(void);                 /* last error message (any descriptor) */
const char *json_filename(int desc);          /* path given to json_load, or NULL    */

int  json_num_registers(int desc);            /* highest index + 1, -1 on error */
int  json_read(int desc, int reg);            /* current value, -1 on error   */
int  json_read_default(int desc, int reg);    /* default value, -1 on error   */
int  json_write(int desc, int reg, unsigned value); /* 0 ok, <0 error         */
int  json_read_all(int desc, unsigned *out, int max); /* # registers copied, -1 on error */
int  json_reset(int desc);                    /* all fields back to default   */

/* Field access by name, base name or tag (see rj_get_field). */
long json_get(int desc, const char *name);    /* value, or <0 error code      */
int  json_set(int desc, const char *name, unsigned value);

int         json_dump(int desc, FILE *out);   /* human readable listing       */
rj_map     *json_map(int desc);               /* underlying rj_map or NULL    */

/* --------------------------------------------------- handle-based API */

rj_map *rj_load(const char *path, char *err, size_t errlen);
rj_map *rj_load_string(const char *json_text, char *err, size_t errlen);
void    rj_free(rj_map *m);

const rj_register *rj_register_info(const rj_map *m, int reg); /* NULL if absent */

int  rj_read(const rj_map *m, int reg);
int  rj_read_default(const rj_map *m, int reg);
int  rj_write(rj_map *m, int reg, unsigned value);
void rj_reset(rj_map *m);

/* Copy current register values of registers 0..max-1 into out[] (absent
 * registers read as 0). Returns number of registers written. */
int  rj_read_all(const rj_map *m, unsigned *out, int max);

/*
 * Logical field access. `name` is matched, in this order, against:
 *   1. the full field name        "dac_biaspa[5:2]", "10bitDAC_threshold"
 *   2. the base name (no [..])    "dac_biaspa", "delay43"
 *   3. the tag                    "discri_threshold", "preamp_bias_dc"
 * All matching parts (possibly in different registers) are combined into
 * one value:
 *   - parts with [a:b] name suffixes are placed at bit a/b relative to
 *     the lowest suffix  (dac_biaspa[5:2] + dac_biaspa[1:0] -> 6 bits)
 *   - otherwise parts are concatenated by tagOrder, 0 = least significant
 *     (10bitDAC_threshold: reg10[7:0] = bits 7..0, reg11[1:0] = bits 9..8)
 * nbits (may be NULL) receives the logical width.
 */
int rj_get_field(const rj_map *m, const char *name, unsigned *value, int *nbits);
int rj_get_field_default(const rj_map *m, const char *name, unsigned *value, int *nbits);
int rj_set_field(rj_map *m, const char *name, unsigned value);

void rj_dump(const rj_map *m, FILE *out);

#ifdef __cplusplus
}
#endif
#endif /* REGJSON_H */
