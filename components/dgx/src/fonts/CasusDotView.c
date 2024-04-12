#include "dgx_font.h"

static const dgx_font_dot_t dots[] = {
    {2, 2},  {1, 3},  {1, 4},  {5, 4},  {5, 5},  {5, 6},  {4, 6},  {3, 7},  {2, 8},  {1, 6},  {1, 5},  {3, 2},  {4, 2},  {5, 3},   //
    {5, 10}, {5, 11}, {5, 9},  {5, 8},  {5, 7},  {4, 12}, {3, 12}, {2, 12}, {1, 11}, {1, 10}, {1, 9},  {1, 8},  {1, 7},            // '0'
    {1, 4},  {2, 3},  {3, 2},  {3, 3},  {3, 4},  {3, 5},  {3, 6},  {3, 7},  {3, 8},  {3, 9},  {3, 10}, {3, 11}, {1, 12}, {2, 12},  //
    {3, 12}, {4, 12}, {5, 12},                                                                                                     // '1'
    {1, 4},  {1, 3},  {2, 2},  {3, 2},  {4, 2},  {5, 3},  {5, 4},  {5, 5},  {5, 6},  {5, 7},  {4, 8},  {3, 9},  {2, 10}, {1, 11},  //
    {1, 12}, {2, 12}, {3, 12}, {4, 12}, {5, 12},                                                                                   // '2'
    {1, 4},  {1, 3},  {2, 2},  {3, 2},  {4, 2},  {5, 3},  {5, 4},  {5, 5},  {5, 6},  {4, 7},  {3, 7},  {5, 8},  {5, 9},  {5, 10},  //
    {5, 11}, {4, 12}, {3, 12}, {2, 12}, {1, 11}, {1, 10},                                                                          // '3'
    {1, 2},  {1, 3},  {1, 4},  {1, 5},  {1, 6},  {1, 7},  {2, 7},  {3, 7},  {4, 7},  {5, 7},  {6, 7},  {5, 3},  {5, 4},  {5, 5},   //
    {5, 6},  {5, 8},  {5, 9},  {5, 10}, {5, 11}, {5, 12},                                                                          // '4'
    {1, 2},  {1, 3},  {1, 4},  {1, 5},  {1, 6},  {1, 7},  {2, 6},  {3, 6},  {4, 6},  {5, 7},  {5, 8},  {2, 2},  {3, 2},  {4, 2},   //
    {5, 2},  {5, 9},  {5, 10}, {5, 11}, {4, 12}, {3, 12}, {2, 12}, {1, 11},                                                        // '5'
    {2, 2},  {1, 3},  {1, 4},  {1, 5},  {1, 6},  {1, 7},  {2, 6},  {3, 6},  {4, 6},  {5, 7},  {5, 8},  {3, 2},  {4, 2},  {5, 3},   //
    {5, 9},  {5, 10}, {5, 11}, {4, 12}, {3, 12}, {2, 12}, {1, 11}, {1, 10}, {1, 9},  {1, 8},                                       // '6'
    {2, 2},  {1, 2},  {1, 3},  {5, 3},  {5, 4},  {5, 5},  {4, 6},  {3, 7},  {3, 8},  {3, 9},  {3, 10}, {3, 2},  {4, 2},  {5, 2},   //
    {3, 11}, {3, 12},                                                                                                              // '7'
    {2, 2},  {1, 3},  {1, 4},  {5, 4},  {5, 5},  {5, 6},  {4, 7},  {3, 7},  {2, 7},  {1, 6},  {1, 5},  {3, 2},  {4, 2},  {5, 3},   //
    {5, 11}, {4, 12}, {5, 10}, {5, 9},  {5, 8},  {3, 12}, {2, 12}, {1, 11}, {1, 10}, {1, 9},  {1, 8},                              // '8'
    {2, 2},  {1, 3},  {1, 4},  {5, 4},  {5, 5},  {5, 6},  {4, 7},  {3, 7},  {2, 7},  {1, 6},  {1, 5},  {3, 2},  {4, 2},  {5, 3},   //
    {5, 10}, {5, 11}, {5, 9},  {5, 8},  {5, 7},  {4, 12}, {3, 12}, {2, 12}, {1, 11},                                               // '9'
};

static const dgx_font_dot_t minus_dots[] = {
    {1, 7}, {2, 7}, {3, 7}, {4, 7}, {5, 7},  // '-'
};

static const dgx_font_dot_t degree_dots[] = {
    {2, 2}, {3, 2}, {4, 3}, {4, 4}, {3, 5}, {2, 5}, {1, 4}, {1, 3},  // '°'
};

static const glyph_t minus_glyph[] = {
    {{.dots = minus_dots, .number_of_dots = 5}, 7, 14, 7, 0, -12}, /* '-' */
};

static const glyph_t degree_glyph[] = {
    {{.dots = degree_dots, .number_of_dots = 8}, 7, 14, 7, 0, -12}, /* '-' */
};

static const glyph_t glyphs[] = {
    {{.dots = dots + 0, .number_of_dots = 27}, 7, 14, 7, 0, -12},   /* '0' */
    {{.dots = dots + 27, .number_of_dots = 17}, 7, 14, 7, 0, -12},  /* '1' */
    {{.dots = dots + 44, .number_of_dots = 19}, 7, 14, 7, 0, -12},  /* '2' */
    {{.dots = dots + 63, .number_of_dots = 20}, 7, 14, 7, 0, -12},  /* '3' */
    {{.dots = dots + 83, .number_of_dots = 20}, 7, 14, 7, 0, -12},  /* '4' */
    {{.dots = dots + 103, .number_of_dots = 22}, 7, 14, 7, 0, -12}, /* '5' */
    {{.dots = dots + 125, .number_of_dots = 24}, 7, 14, 7, 0, -12}, /* '6' */
    {{.dots = dots + 149, .number_of_dots = 16}, 7, 14, 7, 0, -12}, /* '7' */
    {{.dots = dots + 165, .number_of_dots = 25}, 7, 14, 7, 0, -12}, /* '8' */
    {{.dots = dots + 190, .number_of_dots = 23}, 7, 14, 7, 0, -12}, /* '9' */
};

static const glyph_array_t glyph_ranges[] = {
    {0x002D, 1, minus_glyph},   //
    {0x0030, 10, glyphs + 0},   //
    {0x00B0, 1, degree_glyph},  //
    {0x0, 0x0, 0}               //
};

dgx_font_t* CasusDotView() {
    static dgx_font_t rval = {
        .glyph_ranges = glyph_ranges,  //
        .yAdvance = 14,                //
        .yOffsetLowest = -12,          //
        .xWidest = 7,                  //
        .xWidthAverage = 7.000000,     //
        .f_type = DGX_FONT_DOTS        //
    };
    return &rval;
}
