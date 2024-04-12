#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "dgx_arch.h"
#include "drivers/dgx_lcd_init.h"
#include "drivers/st7789.h"
#include "dgx_screen_functions.h"

// ST7789 specific commands used in init
#define ST7789_NOP          0x00
#define ST7789_SWRESET      0x01
#define ST7789_RDDID        0x04
#define ST7789_RDDST        0x09

#define ST7789_RDDPM        0x0A      // Read display power mode
#define ST7789_RDD_MADCTL   0x0B      // Read display MADCTL
#define ST7789_RDD_COLMOD   0x0C      // Read display pixel format
#define ST7789_RDDIM        0x0D      // Read display image mode
#define ST7789_RDDSM        0x0E      // Read display signal mode
#define ST7789_RDDSR        0x0F      // Read display self-diagnostic result (ST7789V)

#define ST7789_SLPIN        0x10
#define ST7789_SLPOUT       0x11
#define ST7789_PTLON        0x12
#define ST7789_NORON        0x13

#define ST7789_INVOFF       0x20
#define ST7789_INVON        0x21
#define ST7789_GAMSET       0x26      // Gamma set
#define ST7789_DISPOFF      0x28
#define ST7789_DISPON       0x29
#define ST7789_CASET        0x2A
#define ST7789_RASET        0x2B
#define ST7789_RAMWR        0x2C
#define ST7789_RGBSET       0x2D      // Color setting for 4096, 64K and 262K colors
#define ST7789_RAMRD        0x2E

#define ST7789_PTLAR        0x30
#define ST7789_VSCRDEF      0x33      // Vertical scrolling definition (ST7789V)
#define ST7789_TEOFF        0x34      // Tearing effect line off
#define ST7789_TEON         0x35      // Tearing effect line on
#define ST7789_MADCTL       0x36      // Memory data access control
#define ST7789_IDMOFF       0x38      // Idle mode off
#define ST7789_IDMON        0x39      // Idle mode on
#define ST7789_RAMWRC       0x3C      // Memory write continue (ST7789V)
#define ST7789_RAMRDC       0x3E      // Memory read continue (ST7789V)
#define ST7789_COLMOD       0x3A

#define ST7789_RAMCTRL      0xB0      // RAM control
#define ST7789_RGBCTRL      0xB1      // RGB control
#define ST7789_PORCTRL      0xB2      // Porch control
#define ST7789_FRCTRL1      0xB3      // Frame rate control
#define ST7789_PARCTRL      0xB5      // Partial mode control
#define ST7789_GCTRL        0xB7      // Gate control
#define ST7789_GTADJ        0xB8      // Gate on timing adjustment
#define ST7789_DGMEN        0xBA      // Digital gamma enable
#define ST7789_VCOMS        0xBB      // VCOMS setting
#define ST7789_LCMCTRL      0xC0      // LCM control
#define ST7789_IDSET        0xC1      // ID setting
#define ST7789_VDVVRHEN     0xC2      // VDV and VRH command enable
#define ST7789_VRHS         0xC3      // VRH set
#define ST7789_VDVSET       0xC4      // VDV setting
#define ST7789_VCMOFSET     0xC5      // VCOMS offset set
#define ST7789_FRCTR2       0xC6      // FR Control 2
#define ST7789_CABCCTRL     0xC7      // CABC control
#define ST7789_REGSEL1      0xC8      // Register value section 1
#define ST7789_REGSEL2      0xCA      // Register value section 2
#define ST7789_PWMFRSEL     0xCC      // PWM frequency selection
#define ST7789_PWCTRL1      0xD0      // Power control 1
#define ST7789_VAPVANEN     0xD2      // Enable VAP/VAN signal output
#define ST7789_CMD2EN       0xDF      // Command 2 enable
#define ST7789_PVGAMCTRL    0xE0      // Positive voltage gamma control
#define ST7789_NVGAMCTRL    0xE1      // Negative voltage gamma control
#define ST7789_DGMLUTR      0xE2      // Digital gamma look-up table for red
#define ST7789_DGMLUTB      0xE3      // Digital gamma look-up table for blue
#define ST7789_GATECTRL     0xE4      // Gate control
#define ST7789_SPI2EN       0xE7      // SPI2 enable
#define ST7789_PWCTRL2      0xE8      // Power control 2
#define ST7789_EQCTRL       0xE9      // Equalize time control
#define ST7789_PROMCTRL     0xEC      // Program control
#define ST7789_PROMEN       0xFA      // Program mode enable
#define ST7789_NVMSET       0xFC      // NVM setting
#define ST7789_PROMACT      0xFE      // Program action

#define ST77XX_MADCTL_MY    0x80
#define ST77XX_MADCTL_MX    0x40
#define ST77XX_MADCTL_MV    0x20
#define ST77XX_MADCTL_ML    0x10
#define ST77XX_MADCTL_RGB   0x00
#define ST77XX_MADCTL_BGR   0x08

typedef struct _dgx_st7789_t {
    dgx_screen_t base;
    dgx_bus_protocols_t *bus;
    gpio_num_t rst;
} dgx_st7789_t;

static const char TAG[] = "DGX ST 7789";

static uint8_t* dgx_st7789_adj_madctl(dgx_screen_with_bus_t *scr, const struct _dgx_lcd_init_cmd_t *init_cmd) {
    static uint8_t adj_data[1];
    if (scr->scr.rgb_order == DgxScreenRGB)
        adj_data[0] = init_cmd->data[0] & ~0x8;
    else
        adj_data[0] = init_cmd->data[0] | 0x08;
    return adj_data;
}

static uint8_t* dgx_st7789_adj_colmod(dgx_screen_with_bus_t *scr, const struct _dgx_lcd_init_cmd_t *init_cmd) {
    static uint8_t adj_data[1];
    if (scr->scr.color_bits == 12)
        adj_data[0] = 0x33;
    else if (scr->scr.color_bits == 18)
        adj_data[0] = 0x66;
    else
        adj_data[0] = 0x55; // 16 Bits
    return adj_data;
}

DRAM_ATTR static const dgx_lcd_init_cmd_t st_init_cmds[] = { { ST7789_SWRESET, 0, 0, 150, { 0 } }, //
        { ST7789_SLPOUT, 0, 0, 255, { 0 } }, //
        { ST7789_COLMOD, dgx_st7789_adj_colmod, 1, 10, { 0x55 } }, // Bits per Pixel: 3 - 12, 5 - 16, 6 - 18
        { ST7789_MADCTL, dgx_st7789_adj_madctl, 1, 0, { 0x00 | 0x00 } }, // MY=0, MX=0, MV=0, ML=0, MH=0 | 00 - RGB, 08 - BGR
        { ST7789_CASET, 0, 4, 0, { 0x00, 0x00, 0x00, 0xF0 } }, //
        { ST7789_RASET, 0, 4, 0, { 0x00, 0x00, 0x00, 0xF0 } }, //
        { ST7789_INVON, 0, 0, 10, { 0 } }, //
        { ST7789_NORON, 0, 0, 10, { 0 } }, //
        { ST7789_DISPON, 0, 0, 255, { 0 } }, //  4: Main screen turn on, no args w/delay
        { 0, 0, 0xff, 0, { 0 } } // Stop
};

//Initialize the display
dgx_screen_t* dgx_st7789_init(dgx_bus_protocols_t *bus, gpio_num_t rst, uint8_t color_bits, dgx_color_order_t cbo) {
    //Attach the LCD to the SPI bus
    dgx_st7789_t *scr = (dgx_st7789_t*) calloc(1, sizeof(dgx_st7789_t));
    if (!scr) {
        ESP_LOGE(TAG, "Screen structure memory allocation failed");
    }
    scr->rst = rst;
    scr->bus = bus;
    scr->base.cg_row_shift = 0;
    scr->base.cg_col_shift = 0;
    scr->base.width = 240;
    scr->base.height = 240;
    scr->base.color_bits = color_bits;
    scr->base.rgb_order = cbo;
    scr->base.screen_name = "ST 7789";
    scr->base.screen_submodel = 0;
    scr->base.screen_subtype = DgxPhysicalScreenWithBus;
    bus->xcmd_set = ST7789_CASET;           //Column Address Set
    bus->ycmd_set = ST7789_RASET;           //Row address set
    bus->wcmd_send = ST7789_RAMWR;           //memory write
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
    dgx_scr_init_slow_bus_optimized_funcs((dgx_screen_t*) scr);
    return (dgx_screen_t*) scr;
}

void dgx_st7789_orientation(dgx_screen_t *_scr, dgx_orientation_t dir_x, dgx_orientation_t dir_y, bool swap_xy) {
    uint8_t cmd = ST7789_MADCTL;
    uint8_t data[1] = { 0 };
    dgx_st7789_t *scr = (dgx_st7789_t*) _scr;
    if (dir_x == DgxScreenRightLeft) data[0] |= ST77XX_MADCTL_MX;
    if (dir_y == DgxScreenBottomTop) {
        data[0] |= ST77XX_MADCTL_MY;
        _scr->cg_row_shift = 80;
    }
    if (swap_xy) data[0] |= ST77XX_MADCTL_MV;
    if (scr->base.rgb_order == DgxScreenBGR) data[0] |= ST77XX_MADCTL_BGR;
    scr->bus->write_command(scr->bus, cmd);
    scr->bus->write_data(scr->bus, data, 1);
}
