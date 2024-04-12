#ifndef __DGX_BITS_H__
#define __DGX_BITS_H__
#ifdef __cplusplus
// @formatter:off
extern "C" {
// @formatter:on
#endif
#include <stdint.h>

#define DGX_ABS(a) ((a) < 0 ? -(a) : (a))
#define DGX_MIN(a, b) ((a) < (b) ? a : b)
#define DGX_MAX(a, b) ((a) < (b) ? b : a)
#define DGX_INT_SWAP(a, b) \
    do                     \
    {                      \
        (a) ^= (b);        \
        (b) ^= (a);        \
        (a) ^= (b);        \
    } while (0)

static inline uint32_t dgx_rol_bits(uint32_t a, uint8_t bits) {
    return (a >> (bits - 1)) | (a << 1);
}

static inline uint32_t dgx_ror_bits(uint32_t a, uint8_t bits) {
    return (a << (bits - 1)) | (a >> 1);
}

static inline uint32_t dgx_rol_nbits(uint32_t a, uint8_t bits, uint8_t n) {
    return (a >> (bits - n)) | (a << n);
}

static inline uint32_t dgx_ror_nbits(uint32_t a, uint8_t bits, uint8_t n) {
    return (a << (bits - n)) | (a >> n);
}

/*
 * Always MSB RGB order
 */

#define DGX_RGB_12(r,g,b) ((((r) & 0xf0u) << 8) | (((g) & 0xf0u) << 4) | ((b) & 0xf0u))
#define DGX_RGB_16(r,g,b) ((((r) & 0xf8u) << 8) | (((g) & 0xfcu) << 3) | (((b) & 0xf8u) >> 3))
#define DGX_RGB_18(r,g,b) ((((r) & 0xfcu) << 16) | (((g) & 0xfcu) << 10) | (((b) & 0xfcu) << 4))
#define DGX_RGB_24(r,g,b) (((r) << 16) | ((g) << 8) | (b))

#define DGX_R_FROM_12(value) ((uint8_t)(((value) >> 8) & 0xf0u))
#define DGX_G_FROM_12(value) ((uint8_t)(((value) >> 4) & 0xf0u))
#define DGX_B_FROM_12(value) ((uint8_t)((value) & 0xf0u))

#define DGX_R_FROM_16(value) ((uint8_t)(((value) >> 8) & 0xf8u))
#define DGX_G_FROM_16(value) ((uint8_t)(((value) >> 3) & 0xfcu))
#define DGX_B_FROM_16(value) ((uint8_t)(((value) << 3) & 0xf8u))

static inline uint32_t dgx_rgb_to_12(uint8_t r, uint8_t g, uint8_t b) {
    return DGX_RGB_12(r,g,b);
}

static inline uint32_t dgx_rgb_to_16(uint8_t r, uint8_t g, uint8_t b) {
    return DGX_RGB_16(r,g,b);
}

static inline uint32_t dgx_rgb_to_18(uint8_t r, uint8_t g, uint8_t b) {
    return DGX_RGB_18(r,g,b);
}

static inline uint32_t dgx_rgb_to_24(uint8_t r, uint8_t g, uint8_t b) {
    return DGX_RGB_24(r,g,b);
}

static inline uint8_t* dgx_fill_buf_value_1(uint8_t *lp, int idx, uint32_t value) {
    uint8_t mask = 0x80 >> (idx & 7);
    if (value) {
        *lp |= mask;
    } else {
        *lp &= ~mask;
    }
    lp += mask & 1;
    return lp;
}

static inline uint8_t* dgx_fill_buf_value_4(uint8_t *lp, int idx, uint32_t value) {
    /*
     * mask:
     *    0f when idx is even
     *    f0 when idx is odd
     */
    uint8_t idx_shift = (idx & 1) << 2;
    uint8_t mask = 0x0fu << idx_shift;
    uint8_t pb = *lp & mask;
    *lp = pb | ((uint8_t)value >> idx_shift);
    lp += idx & 1;
    return lp;
}

static inline uint8_t* dgx_fill_buf_value_8(uint8_t *lp, int idx, uint32_t value) {
    *lp = (uint8_t)value;
    return lp + 1;
}

static inline uint8_t* dgx_fill_buf_value_12(uint8_t *lp, int idx, uint32_t value) {
    if ((idx & 1) == 0) {
        *lp++ = (uint8_t)(value >> 8);
        *lp = (uint8_t)(value & 0xf0u);
    } else {
        uint8_t pb = *lp & 0xf0u;
        *lp++ = pb | (uint8_t)(value >> 12);
        *lp++ = (uint8_t)(value >> 4);
    }
    return lp;
}

static inline uint8_t* dgx_fill_buf_value_16(uint8_t *lp, int idx, uint32_t value) {
    lp[0] = (uint8_t)((value & 0xff00u)>> 8);
    lp[1] = (uint8_t)(value & 0xffu);
    return lp + 2;
}

static inline uint8_t* dgx_fill_buf_value_24(uint8_t *lp, int idx, uint32_t value) {
    *lp++ = (uint8_t)(value >> 16);
    *lp++ = (uint8_t)(value >> 8);
    *lp++ = (uint8_t)value;
    return lp;
}

static inline uint8_t* dgx_fill_buf_value_32(uint8_t *lp, int idx, uint32_t value) {
    *lp++ = (uint8_t)(value >> 24);
    *lp++ = (uint8_t)(value >> 16);
    *lp++ = (uint8_t)(value >> 8);
    *lp++ = (uint8_t)value;
    return lp;
}

static inline uint32_t dgx_read_buf_value_1(uint8_t **lp, int idx) {
    uint8_t mask = 0x80 >> (idx & 7);
    uint32_t value = !!(**lp & mask);
    *lp += mask & 1;
    return value;
}

static inline uint32_t dgx_read_buf_value_4(uint8_t **lp, int idx) {
    uint8_t value = **lp << 8;
    *lp += idx & 1;
    if (idx & 1) {
        value <<= 4;
    }
    return value;
}

static inline uint32_t dgx_read_buf_value_8(uint8_t **lp, int idx) {
    uint32_t value = **lp;
    ++*lp;
    return value;
}

static inline uint32_t dgx_read_buf_value_12(uint8_t **lp, int idx) {
    uint32_t value = **lp << 8;
    ++*lp;
    if ((idx & 1) == 0) {
        value |= **lp & 0xf0;
    } else {
        value |= **lp;
        value <<= 4;
        value &= 0xfff0;
        ++*lp;
    }
    return value;
}

static inline uint32_t dgx_read_buf_value_16(uint8_t **lp, int idx) {
    uint32_t value = (**lp << 8) | (*(*lp+1));
    *lp += 2;
    return value;
}

static inline uint32_t dgx_read_buf_value_24(uint8_t **lp, int idx) {
    uint32_t value = (**lp << 16) | (*(*lp+1) << 8) | (*(*lp+2));
    *lp += 3;
    return value;
}

static inline uint32_t dgx_read_buf_value_32(uint8_t **lp, int idx) {
    uint32_t value = (**lp << 24) | (*(*lp+1) << 16) | (*(*lp+2)<<8) | (*(*lp+3));
    *lp += 4;
    return value;
}

static inline uint32_t dgx_read_buf_value(uint8_t color_bits, uint8_t **lp, int idx) {
    switch(color_bits) {
        case 1: return dgx_read_buf_value_1(lp, idx);
        case 4: return dgx_read_buf_value_4(lp, idx);
        case 8: return dgx_read_buf_value_8(lp, idx);
        case 12: return dgx_read_buf_value_12(lp, idx);
        case 16: return dgx_read_buf_value_16(lp, idx);
        case 18:
        case 24: return dgx_read_buf_value_24(lp, idx);
        case 32: return dgx_read_buf_value_32(lp, idx);
        default: return 0;
    }
}

static inline uint8_t* dgx_fill_buf_value(uint8_t color_bits, uint8_t *lp, int idx, uint32_t value) {
    switch(color_bits) {
        case 1: return dgx_fill_buf_value_1(lp, idx, value);
        case 4: return dgx_fill_buf_value_4(lp, idx, value);
        case 8: return dgx_fill_buf_value_8(lp, idx, value);
        case 12: return dgx_fill_buf_value_12(lp, idx, value);
        case 16: return dgx_fill_buf_value_16(lp, idx, value);
        case 18:
        case 24: return dgx_fill_buf_value_24(lp, idx, value);
        case 32: return dgx_fill_buf_value_32(lp, idx, value);
        default: return 0;
    }
}

static inline uint32_t dgx_bytes_to_color_points(uint8_t color_bits, uint32_t bc) {
    switch(color_bits) {
        case 1: return bc * 8;
        case 4: return bc * 2;
        case 8: return bc;
        case 12: return bc * 2 / 3;
        case 16: return bc / 2;
        case 18:
        case 24: return bc / 3;
        case 32: return bc / 4;
        default: return 0;
    }
}

static inline uint32_t dgx_color_points_to_bytes(uint8_t color_bits, uint32_t cp) {
    switch(color_bits) {
        case 1: return (cp + 7u)/ 8u;
        case 4: return (cp + 1u) / 2u;
        case 8: return cp;
        case 12: return (cp * 3u + 1u) / 2u;
        case 16: return cp * 2u;
        case 18:
        case 24: return cp * 3u;
        case 32: return cp * 4u;
        default: return 0;
    }
}

static inline uint32_t dgx_color_bits_to_bytes(uint8_t color_bits) {
    return (color_bits + 7u) >> 3;
}

#define DGX_FILL_BUFFER(color_bits, bpointer, pic, color) \
    do {\
        switch (color_bits) {\
            case 1:\
                for (int i = 0; i < pic; ++i)\
                    bpointer = dgx_fill_buf_value_1(bpointer, i, color);\
                break;\
            case 4:\
                for (int i = 0; i < pic; ++i)\
                    bpointer = dgx_fill_buf_value_4(bpointer, i, color);\
                break;\
            case 8:\
                for (int i = 0; i < pic; ++i)\
                    bpointer = dgx_fill_buf_value_8(bpointer, i, color);\
                break;\
            case 12:\
                for (int i = 0; i < pic; ++i)\
                    bpointer = dgx_fill_buf_value_12(bpointer, i, color);\
                break;\
            case 16:\
                for (int i = 0; i < pic; ++i)\
                    bpointer = dgx_fill_buf_value_16(bpointer, i, color);\
                break;\
            case 18:\
            case 24:\
                for (int i = 0; i < pic; ++i)\
                    bpointer = dgx_fill_buf_value_24(bpointer, i, color);\
                break;\
            case 32:\
                for (int i = 0; i < pic; ++i)\
                    bpointer = dgx_fill_buf_value_32(bpointer, i, color);\
                break;\
            default:\
                return;\
        }\
    } while(0)


#ifdef __cplusplus
// @formatter:off
}
// @formatter:on

#endif

#endif /* __DGX_BITS_H__ */
