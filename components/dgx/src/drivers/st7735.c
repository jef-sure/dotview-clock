#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "dgx_arch.h"
#include "drivers/dgx_lcd_init.h"
#include "drivers/st7735.h"
#include "dgx_screen_functions.h"

// ST7735 specific commands used in init
#define ST7735_NOP          0x00
#define ST7735_SWRESET      0x01
#define ST7735_RDDID        0x04
#define ST7735_RDDST        0x09

#define ST7735_SLPIN        0x10
#define ST7735_SLPOUT       0x11
#define ST7735_PTLON        0x12
#define ST7735_NORON        0x13

#define ST7735_INVOFF       0x20
#define ST7735_INVON        0x21
#define ST7735_DISPOFF      0x28
#define ST7735_DISPON       0x29
#define ST7735_CASET        0x2A
#define ST7735_RASET        0x2B
#define ST7735_RAMWR        0x2C
#define ST7735_RAMRD        0x2E

#define ST7735_PTLAR        0x30
#define ST7735_VSCRDEF      0x33
#define ST7735_COLMOD       0x3A
#define ST7735_MADCTL       0x36
#define ST7735_VSCRSADD     0x37

#define ST7735_FRMCTR1      0xB1
#define ST7735_FRMCTR2      0xB2
#define ST7735_FRMCTR3      0xB3
#define ST7735_INVCTR       0xB4
#define ST7735_DISSET5      0xB6

#define ST7735_PWCTR1       0xC0
#define ST7735_PWCTR2       0xC1
#define ST7735_PWCTR3       0xC2
#define ST7735_PWCTR4       0xC3
#define ST7735_PWCTR5       0xC4
#define ST7735_VMCTR1       0xC5

#define ST7735_RDID1        0xDA
#define ST7735_RDID2        0xDB
#define ST7735_RDID3        0xDC
#define ST7735_RDID4        0xDD

#define ST7735_PWCTR6       0xFC

#define ST7735_GMCTRP1      0xE0
#define ST7735_GMCTRN1      0xE1

#define ST77XX_MADCTL_MY    0x80
#define ST77XX_MADCTL_MX    0x40
#define ST77XX_MADCTL_MV    0x20
#define ST77XX_MADCTL_ML    0x10
#define ST77XX_MADCTL_RGB   0x00
#define ST77XX_MADCTL_BGR   0x08

typedef struct _dgx_st7735_t {
    dgx_screen_t base;
    dgx_bus_protocols_t *bus;
    gpio_num_t rst;
} dgx_st7735_t;

static const char TAG[] = "DGX ST 7735";

static uint8_t* dgx_st7735_adj_madctl(dgx_screen_with_bus_t *scr, const struct _dgx_lcd_init_cmd_t *init_cmd) {
    static uint8_t adj_data[1];
    if (scr->scr.rgb_order == DgxScreenRGB)
        adj_data[0] = init_cmd->data[0] & ~0x8;
    else
        adj_data[0] = init_cmd->data[0] | 0x08;
    return adj_data;
}

static uint8_t* dgx_st7735_adj_colmod(dgx_screen_with_bus_t *scr, const struct _dgx_lcd_init_cmd_t *init_cmd) {
    static uint8_t adj_data[1];
    if (scr->scr.color_bits == 12)
        adj_data[0] = 0x03;
    else if (scr->scr.color_bits == 18)
        adj_data[0] = 0x06;
    else
        adj_data[0] = 0x05; // 16 Bits
    return adj_data;
}

DRAM_ATTR static const dgx_lcd_init_cmd_t st_init_cmds[] = {
//
        { ST7735_SWRESET, 0, 0, 150, { 0 } }, //
        { ST7735_SLPOUT, 0, 0, 255, { 0 } }, //
        { ST7735_FRMCTR1, 0, 3, 0, { 0x01, 0x2C, 0x2D } }, //
        { ST7735_FRMCTR2, 0, 3, 0, { 0x01, 0x2C, 0x2D } }, //
        { ST7735_FRMCTR3, 0, 6, 0, { 0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D } }, //
        { ST7735_INVCTR, 0, 1, 0, { 0x07 } }, //
        { ST7735_PWCTR1, 0, 3, 0, { 0xA2, 0x02, 0x84 } }, //
        { ST7735_PWCTR2, 0, 1, 0, { 0xC5 } }, //
        { ST7735_PWCTR3, 0, 2, 0, { 0x0A, 0x00 } }, //
        { ST7735_PWCTR4, 0, 2, 0, { 0x8A, 0x2A } }, //
        { ST7735_PWCTR5, 0, 2, 0, { 0x8A, 0xEE } }, //
        { ST7735_VMCTR1, 0, 1, 9, { 0x0E } }, //
        { ST7735_MADCTL, dgx_st7735_adj_madctl, 1, 0, { 0x00 | 0x00 } }, // MY=0, MX=0, MV=0, ML=0, MH=0 | 00 - RGB, 08 - BGR
        { ST7735_COLMOD, dgx_st7735_adj_colmod, 1, 0, { 0x06 } }, // Bits per Pixel: 3 - 12, 5 - 16, 6 - 18
        { ST7735_CASET, 0, 4, 0, { 0x00, 0x00, 0x00, 0x7F } }, //
        { ST7735_RASET, 0, 4, 0, { 0x00, 0x00, 0x00, 0x9F } }, //
        { ST7735_GMCTRP1, 0, 16, 0, { 0x02, 0x1c, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2d, 0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10 } }, //
        { ST7735_GMCTRN1, 0, 16, 0, { 0x03, 0x1d, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D, 0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10 } }, //
        { ST7735_NORON, 0, 0, 10, { 0 } }, //
        { ST7735_DISPON, 0, 0, 100, { 0 } }, //  4: Main screen turn on, no args w/delay
        { 0, 0, 0xff, 0, { 0 } } // Stop
};

//Initialize the display
dgx_screen_t* dgx_st7735_init(dgx_bus_protocols_t *bus, gpio_num_t rst, uint8_t color_bits, dgx_color_order_t cbo) {
    dgx_st7735_t *scr = (dgx_st7735_t*) calloc(1, sizeof(dgx_st7735_t));
    if (!scr) {
        ESP_LOGE(TAG, "Screen structure memory allocation failed");
    }
    scr->rst = rst;
    scr->bus = bus;
    scr->base.cg_row_shift = 0;
    scr->base.cg_col_shift = 0;
    scr->base.width = 128;
    scr->base.height = 160;
    scr->base.color_bits = color_bits;
    scr->base.rgb_order = cbo;
    scr->base.screen_name = "ST 7735";
    scr->base.screen_submodel = 0;
    scr->base.screen_subtype = DgxPhysicalScreenWithBus;
    bus->xcmd_set = ST7735_CASET;           //Column Address Set
    bus->ycmd_set = ST7735_RASET;           //Row address set
    bus->wcmd_send = ST7735_RAMWR;           //memory write
    // Memory read is not supported yet
    bus->rcmd_send = 0; // ST7735_RAMRD;           //memory read
//Initialize non-SPI GPIOs
    dgx_gpio_set_direction(rst, GPIO_MODE_OUTPUT);
//Reset the display
    dgx_gpio_set_level(rst, 0);
    dgx_delay(100);
    dgx_gpio_set_level(rst, 1);
    dgx_delay(100);
//Send all the commands
    dgx_lcd_init((dgx_screen_with_bus_t*) scr, st_init_cmds);
    dgx_scr_init_slow_bus_optimized_funcs((dgx_screen_t*)scr);
    return (dgx_screen_t*) scr;
}

void dgx_st7735_orientation(dgx_screen_t *_scr, dgx_orientation_t dir_x, dgx_orientation_t dir_y, bool swap_xy) {
    dgx_st7735_t *scr = (dgx_st7735_t*) _scr;
    uint8_t cmd = ST7735_MADCTL;
    uint8_t data[1] = { 0 };
    if (dir_x == DgxScreenRightLeft) data[0] |= ST77XX_MADCTL_MX;
    if (dir_y == DgxScreenBottomTop) data[0] |= ST77XX_MADCTL_MY;
    if (scr->base.rgb_order == DgxScreenBGR) data[0] |= ST77XX_MADCTL_BGR;
    if (swap_xy) data[0] |= ST77XX_MADCTL_MV;
    scr->bus->write_command(scr->bus, cmd);
    scr->bus->write_data(scr->bus, data, 1);
}
