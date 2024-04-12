#include <assert.h>
#include "str.h"
#include <esp_log.h>
#include "esp_heap_caps.h"

static const char TAG[] = "STR_H";

str_t *str_new_ln(size_t ln) {
    str_t *ret;
    size_t cap = _str_allign(ln);
    ret = (str_t *)malloc(sizeof(str_t) + cap);
    if (!ret) {
        ESP_LOGE(TAG, "str_new_ln: impossible to allocate %u bytes, ln: %u, cap: %u", sizeof(str_t) + cap, ln, cap);
        heap_caps_print_heap_info(MALLOC_CAP_8BIT);
        ret = (str_t *)heap_caps_malloc(sizeof(str_t) + cap, MALLOC_CAP_8BIT);
        if (!ret) {
            ESP_LOGE(TAG, "str_new_ln: impossible to allocate even with heap_caps_malloc %u bytes, ln: %u, cap: %u", sizeof(str_t) + cap, ln, cap);
            return 0;
        }
    }
    // assert(ret);
    ret->length = ln;
    ret->capacity = cap;
    return ret;
}

str_t *str_new_pc(const char *pc) {
    size_t sln = strlen(pc);
    str_t *ret = str_new_ln(sln);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#pragma GCC diagnostic ignored "-Wstringop-overflow"
    memcpy(ret->data, pc, sln);
#pragma GCC diagnostic pop
    return ret;
}

str_t *str_new_pcln(const char *pc, size_t ln) {
    str_t *ret = str_new_ln(ln);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#pragma GCC diagnostic ignored "-Wstringop-overflow"
    memcpy(ret->data, pc, ln);
#pragma GCC diagnostic pop
    return ret;
}

str_t *str_resize(str_t **dest_str, size_t ln) {
    if (dest_str == NULL) {
        ESP_LOGE(TAG, "str_reserve_ln: dst_str is NULL");
        return 0;
    }
    if (*dest_str == NULL) {
        *dest_str = str_new_ln(ln);
        (*dest_str)->length = 0;
        return *dest_str;
    }
    if ((*dest_str)->capacity >= ln && (*dest_str)->capacity < ln * 2) {
        // the string already has enough capacity and not "too much"
        return *dest_str;
    }
    size_t cap = _str_allign(ln);
    size_t new_ln = (*dest_str)->length < ln ? (*dest_str)->length : ln;
    str_t *ret = (str_t *)realloc(*dest_str, sizeof(str_t) + cap);
    if (!ret) {
        ESP_LOGE(TAG, "str_resize: impossible to allocate %u bytes, ln: %u, cap: %u", sizeof(str_t) + cap, ln, cap);
        heap_caps_print_heap_info(MALLOC_CAP_8BIT);
        ret = (str_t *)heap_caps_realloc(*dest_str, sizeof(str_t) + cap, MALLOC_CAP_8BIT);
        if (!ret) {
            ESP_LOGE(TAG, "str_resize: impossible to heap_caps_realloc %u bytes, ln: %u, cap: %u", sizeof(str_t) + cap, ln, cap);
            return 0;
        }
    }
    ret->length = new_ln;
    ret->capacity = cap;
    *dest_str = ret;
    return ret;
}

bool str_is_empty(const str_t *str) {
    size_t r;
    if (str_length(str) == 0) {
        return 1;
    }
    for (r = 0; r < str_length(str) && isspace((uint8_t)str->data[r]); ++r)
        ;
    if (r == str->length) {
        return 1;  // true
    }
    return 0;  // false
}

void str_alltrim(str_t **dst_str) {
    size_t p1, p2;
    str_t *tmp;
    for (p1 = 0; p1 < (*dst_str)->length && isspace((uint8_t)(*dst_str)->data[p1]); ++p1)
        ;
    if (p1 == (*dst_str)->length) {
        (*dst_str)->length = 0;
        return;
    }
    for (p2 = (*dst_str)->length - 1; p2 + 1 > p1 && isspace((uint8_t)(*dst_str)->data[p2]); --p2)
        ;
    if (p1 == 0 && p2 + 1 == (*dst_str)->length) return;
    tmp = str_substr(*dst_str, p1, p2 - p1 + 1);
    str_destroy(dst_str);
    *dst_str = tmp;
}

void str_append_pcln(str_t **dest_str, const char *pc, size_t sln) {
    if (pc == NULL || sln == 0) {
        return;
    }
    if (*dest_str == NULL)
        *dest_str = str_new_pcln(pc, sln);
    else {
        if ((*dest_str)->capacity < (*dest_str)->length + sln) {
            str_t *tmp = str_resize(dest_str, (*dest_str)->length + sln);
            if (!tmp) {
                ESP_LOGE(TAG, "str_append_pcln: impossible to allocate additional %u bytes, toatal: %u", sln, (*dest_str)->length + sln);
                return;
            }
        }
        memcpy((*dest_str)->data + (*dest_str)->length, pc, sln);
        (*dest_str)->length += sln;
    }
}

void str_append_c(str_t **dest_str, char c) {
    if (dest_str == NULL) {
        return;
    }
    if (*dest_str == NULL) {
        *dest_str = str_new_ln(1);
        if (*dest_str) {
            (*dest_str)->data[0] = c;
        }
    } else {
        if ((*dest_str)->capacity < (*dest_str)->length + 1) {
            const size_t sln = 1u;
            str_t *tmp = str_resize(dest_str, (*dest_str)->length + sln);
            if (!tmp) {
                ESP_LOGE(TAG, "str_append_c: impossible to allocate additional %u bytes, toatal: %u", sln, (*dest_str)->length + sln);
                return;
            }
        }
        (*dest_str)->data[(*dest_str)->length] = c;
        (*dest_str)->length += 1;
    }
}

void str_replace_str(str_t **dest_str, size_t pos, size_t n, const str_t *str) {
    size_t nl;
    str_t *tmp;
    if (pos == (*dest_str)->length) {
        str_append_str(dest_str, str);
        return;
    }
    if (pos > (*dest_str)->length) {
        return;
    }
    if (str == NULL) str = &str_empty;
    if (n == str_npos || pos + n > (*dest_str)->length) n = (*dest_str)->length - pos;
    nl = (*dest_str)->length + str->length - n;
    tmp = str_new_ln(nl);
    if (pos) memcpy(tmp->data, (*dest_str)->data, pos);
    if (str->length) memcpy(tmp->data + pos, str->data, str->length);
    if (pos + n < (*dest_str)->length) memcpy(tmp->data + pos + str->length, (*dest_str)->data + pos + n, (*dest_str)->length - n - pos);
    str_destroy(dest_str);
    *dest_str = tmp;
}

size_t str_find_str(const str_t *str, const str_t *substr, size_t pos) {
    if (str == NULL) return str_npos;
    if (substr->length > str->length || str->length == 0 || substr->length == 0) {
        return str_npos;
    }
    while (pos + substr->length <= str->length)
        if (memcmp(str->data + pos, substr->data, substr->length) == 0) {
            return pos;
        } else
            ++pos;
    return str_npos;
}

size_t str_find_pc(const str_t *str, const char *pc, size_t pos) {
    size_t sln = strlen(pc);
    if (str == NULL) return str_npos;
    if (sln > str->length || str->length == 0 || sln == 0) {
        return str_npos;
    }
    while (pos + sln <= str->length)
        if (memcmp(str->data + pos, pc, sln) == 0) {
            return pos;
        } else
            ++pos;
    return str_npos;
}

size_t str_find_c(const str_t *str, char c, size_t pos) {
    char *cptr;
    if (str == NULL) return str_npos;
    if (pos >= str->length || str->length == 0) {
        return str_npos;
    }
    cptr = (char *)memchr(str->data + pos, c, str->length - pos);
    if (cptr != NULL) {
        return cptr - str->data;
    }
    return str_npos;
}

size_t str_rfind_c(const str_t *str, char c, size_t pos) {
    if (str == NULL) return str_npos;
    if (str->length == 0) {
        return str_npos;
    }
    if (pos >= str->length) pos = str->length - 1;
    while (pos + 1 > 0) {
        if (str->data[pos] == c) {
            return pos;
        }
        --pos;
    }
    return str_npos;
}

size_t str_rfind_str(const str_t *str, const str_t *substr, size_t pos) {
    if (str == NULL) return str_npos;
    if (substr->length > str->length || str->length == 0 || substr->length == 0) {
        return str_npos;
    }
    if (pos == str_npos || pos + substr->length > str->length) pos = str->length - substr->length;
    while (pos + 1 > 0)
        if (memcmp(str->data + pos, substr->data, substr->length) == 0) {
            return pos;
        } else
            --pos;
    return str_npos;
}

size_t str_rfind_pc(const str_t *str, const char *pc, size_t pos) {
    size_t sln = strlen(pc);
    if (str == NULL) return str_npos;
    if (sln > str->length || str->length == 0 || sln == 0) {
        return str_npos;
    }
    if (pos == str_npos || pos + sln > str->length) pos = str->length - sln;
    while (pos + 1 > 0)
        if (memcmp(str->data + pos, pc, sln) == 0) {
            return pos;
        } else
            --pos;
    return str_npos;
}

str_vector_t *str_vector_new(size_t ln) {
    str_vector_t *ret = (str_vector_t *)malloc(sizeof(*ret));
    assert(ret);
    ret->vector = (str_t **)calloc(ln, sizeof(str_t *));
    ret->length = ln;
    return ret;
}

void str_vector_destroy(str_vector_t **pstrv) {
    if (*pstrv != NULL) {
        size_t n;
        for (n = 0; n < (*pstrv)->length; ++n) str_destroy((*pstrv)->vector + n);
        free((*pstrv)->vector);
        free(*pstrv);
        *pstrv = NULL;
    }
}

str_vector_t *str_split_pc(const str_t *str, const char *pc) {
    size_t sln;
    str_vector_t *ret = str_vector_new(0);
    if (str_length(str) == 0) {
        str_vector_add_str(ret, str_new_ln(0));
        return ret;
    }
    if (pc == NULL || (sln = strlen(pc)) == 0) {
        size_t i;
        str_vector_destroy(&ret);
        ret = str_vector_new(str->length);
        for (i = 0; i < str->length; ++i) ret->vector[i] = str_new_pcln(str->data + i, 1);
        return ret;
    } else {
        size_t npos = 0, fpos;
        do {
            fpos = str_find_pc(str, pc, npos);
            if (fpos != str_npos) {
                str_vector_add_str(ret, str_substr(str, npos, fpos - npos));
            } else {
                str_vector_add_str(ret, str_substr(str, npos, str->length - npos));
                break;
            }
            npos = fpos + sln;
        } while (1);
        return ret;
    }
}
