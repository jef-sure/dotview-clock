/*
 * str.h
 *
 *  Created on: Mar 16, 2021
 *      Author: anton
 */

#ifndef _STR_H_
#define _STR_H_

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct _str_t {
    size_t capacity;
    size_t length;
    char data[1];
} str_t;

typedef struct _str_vector_t {
    size_t length;
    str_t **vector;
} str_vector_t;

static inline size_t _str_allign(size_t ln) {
    ln += 4 - ((ln + sizeof(str_t)) & 3);
    return ln;
}

static const size_t str_npos = (size_t)(-1);
static const str_t str_empty = {1u, 0u, {0}};

str_t *str_new_ln(size_t ln);

str_t *str_new_pc(const char *pc);

str_t *str_new_pcln(const char *pc, size_t ln);

static inline str_t *str_new_str(const str_t *str) {  //
    return str_new_pcln(str->data, str->length);      //
}

str_t *str_resize(str_t **dest_str, size_t ln);

static inline void str_destroy(str_t **dst) {
    if (dst) {
        free(*dst);
        *dst = NULL;
    }
}

static inline void str_clear(str_t *str) {  //
    str->length = 0;                        //
}

static inline char *str_c(str_t *str) {
    str->data[str->length] = 0;
    return str->data;
}

static inline size_t str_length(const str_t *str) {
    if (str != NULL) {
        return str->length;
    } else {
        return 0;
    }
}

static inline size_t str_capacity(const str_t *str) {
    if (str != NULL) {
        return str->capacity;
    } else {
        return 0;
    }
}

bool str_is_empty(const str_t *str);

static inline bool str_xeq_pc(const str_t *str, const char *pc) {
    size_t sln = strlen(pc);
    if (str->length < sln) {
        return 0;  // false
    }
    return memcmp(str->data + (str->length - sln), pc, sln) == 0;
}

static inline int str_cmp_str(const str_t *str, const str_t *str_b) {
    size_t cln = str_b->length > str->length ? str->length : str_b->length;
    int r = memcmp(str->data, str_b->data, cln);
    if (r) return r;
    if (str_b->length == str->length) return 0;
    if (str_b->length > str->length) return -1;
    return 1;
}

static inline int str_cmp_pc(const str_t *str, const char *pc) {
    size_t sln = strlen(pc);
    size_t cln = sln > str->length ? str->length : sln;
    int r = memcmp(str->data, pc, cln);
    if (r) return r;
    if (sln == str->length) return 0;
    if (sln > str->length) return -1;
    return 1;
}

static inline str_t *str_substr(const str_t *str, size_t pos, size_t len) {
    if (pos >= str->length || len == 0) {
        return str_new_ln(0);
    }
    if (len == str_npos || pos + len > str->length) {
        len = str->length - pos;
    }
    return str_new_pcln(str->data + pos, len);
}

void str_alltrim(str_t **dst_str);

static inline void str_chomp(str_t *str) {
    char c;
    if (str != NULL)
        while (str->length != 0 && ((c = str->data[str->length - 1]) == '\r' || c == '\n')) --str->length;
}

static inline void str_tolower(str_t *str) {
    size_t i;
    for (i = 0; i < str->length; ++i) str->data[i] = tolower(str->data[i]);
}

static inline void str_toupper(str_t *str) {
    size_t i;
    for (i = 0; i < str->length; ++i) str->data[i] = toupper(str->data[i]);
}

void str_append_pcln(str_t **dest_str, const char *pc, size_t sln);

static inline void str_append_str(str_t **dest_str, const str_t *str) {
    if (str == NULL || str->length == 0) {
        return;
    }
    if (*dest_str == NULL)
        *dest_str = str_new_str(str);
    else
        str_append_pcln(dest_str, str->data, str->length);
}

static inline void str_append_pc(str_t **dest_str, const char *pc) {
    size_t sln;
    if (pc == NULL || (sln = strlen(pc)) == 0) {
        return;
    }
    str_append_pcln(dest_str, pc, sln);
}

void str_append_c(str_t **dest_str, char c);

void str_replace_str(str_t **dest_str, size_t pos, size_t n, const str_t *str);

size_t str_find_str(const str_t *str, const str_t *substr, size_t pos);

size_t str_find_pc(const str_t *str, const char *pc, size_t pos);

size_t str_find_c(const str_t *str, char c, size_t pos);

size_t str_rfind_c(const str_t *str, char c, size_t pos);

size_t str_rfind_str(const str_t *str, const str_t *substr, size_t pos);

size_t str_rfind_pc(const str_t *str, const char *pc, size_t pos);

str_vector_t *str_vector_new(size_t ln);

static inline void str_vector_add_str(str_vector_t *strv, str_t *str) {
    strv->vector = (str_t **)realloc(strv->vector, (strv->length + 1) * sizeof(str_t *));
    strv->vector[strv->length] = str;
    ++strv->length;
}

static inline void str_vector_del_str(str_vector_t *strv) {
    str_destroy(strv->vector + strv->length - 1);
    --strv->length;
}

void str_vector_destroy(str_vector_t **pstrv);

str_vector_t *str_split_pc(const str_t *str, const char *pc);

#endif /* _STR_H_ */
