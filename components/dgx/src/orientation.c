#include <stdbool.h>
#include <stdint.h>
#include "dgx_screen.h"

dgx_point_2d_t _dgx_start_area_pixel(  //
    int left,                                //
    int right,                               //
    int top,                                 //
    int bottom,                              //
    dgx_orientation_t dir_x,                 //
    dgx_orientation_t dir_y                  //
) {
    dgx_point_2d_t ret;
    if (dir_x == DgxScreenLeftRight)
        ret.x = left;
    else
        ret.x = right;
    if (dir_y == DgxScreenTopBottom)
        ret.y = top;
    else
        ret.y = bottom;
    return ret;
}

dgx_point_2d_t _dgx_move_to_next_area_pixel(  //
    dgx_point_2d_t current_point,                    //
    int left,                                       //
    int right,                                      //
    int top,                                        //
    int bottom,                                     //
    dgx_orientation_t dir_x,                        //
    dgx_orientation_t dir_y,                        //
    bool swap_xy                                    //
) {
    dgx_point_2d_t ret = {.x = current_point.x, .y = current_point.y};
    if (swap_xy == false) {
        if (dir_x == DgxScreenLeftRight)
            if (ret.x >= right) {
                ret.x = left;
                if (dir_y == DgxScreenTopBottom) {
                    if (ret.y >= bottom) {
                        ret.y = top;
                    } else {
                        ret.y++;
                    }
                } else {
                    if (ret.y <= top) {
                        ret.y = bottom;
                    } else {
                        ret.y--;
                    }
                }
            } else {
                ret.x++;
            }
        else {  // DgxScreenRightLeft
            if (ret.x <= left) {
                ret.x = right;
                if (dir_y == DgxScreenTopBottom) {
                    if (ret.y >= bottom) {
                        ret.y = top;
                    } else {
                        ret.y++;
                    }
                } else {  // DgxScreenBottomTop
                    if (ret.y <= top) {
                        ret.y = bottom;
                    } else {
                        ret.y--;
                    }
                }
            } else {
                ret.x--;
            }
        }
    } else {  // swap x-y
        if (dir_y == DgxScreenTopBottom) {
            if (ret.y >= bottom) {
                ret.y = top;
                if (dir_x == DgxScreenLeftRight) {
                    if (ret.x >= right) {
                        ret.x = left;
                    } else {
                        ret.x++;
                    }
                } else {  // DgxScreenRightLeft
                    if (ret.x <= left) {
                        ret.x = right;
                    } else {
                        ret.x--;
                    }
                }
            } else {
                ret.y++;
            }
        } else {  // DgxScreenBottomTop
            if (ret.y <= top) {
                ret.y = bottom;
                if (dir_x == DgxScreenLeftRight) {
                    if (ret.x >= right) {
                        ret.x = left;
                    } else {
                        ret.x++;
                    }
                } else {  // DgxScreenRightLeft
                    if (ret.x <= left) {
                        ret.x = right;
                    } else {
                        ret.x--;
                    }
                }
            } else {
                ret.y--;
            }
        }
    }
    return ret;
}
