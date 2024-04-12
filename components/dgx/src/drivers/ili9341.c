#include "drivers/ili9341.h"

#include <string.h>

#include "dgx_arch.h"
#include "dgx_screen_functions.h"
#include "drivers/dgx_lcd_init.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define ILI9341_TFTWIDTH 240   ///< ILI9341 max TFT width
#define ILI9341_TFTHEIGHT 320  ///< ILI9341 max TFT height

#define ILI9341_NOP 0x00      ///< No-op register
#define ILI9341_SWRESET 0x01  ///< Software reset register
#define ILI9341_RDDID 0x04    ///< Read display identification information
#define ILI9341_RDDST 0x09    ///< Read Display Status

#define ILI9341_SLPIN 0x10   ///< Enter Sleep Mode
#define ILI9341_SLPOUT 0x11  ///< Sleep Out
#define ILI9341_PTLON 0x12   ///< Partial Mode ON
#define ILI9341_NORON 0x13   ///< Normal Display Mode ON

#define ILI9341_RDMODE 0x0A      ///< Read Display Power Mode
#define ILI9341_RDMADCTL 0x0B    ///< Read Display MADCTL
#define ILI9341_RDPIXFMT 0x0C    ///< Read Display Pixel Format
#define ILI9341_RDIMGFMT 0x0D    ///< Read Display Image Format
#define ILI9341_RDSELFDIAG 0x0F  ///< Read Display Self-Diagnostic Result

#define ILI9341_INVOFF 0x20    ///< Display Inversion OFF
#define ILI9341_INVON 0x21     ///< Display Inversion ON
#define ILI9341_GAMMASET 0x26  ///< Gamma Set
#define ILI9341_DISPOFF 0x28   ///< Display OFF
#define ILI9341_DISPON 0x29    ///< Display ON

#define ILI9341_CASET 0x2A  ///< Column Address Set
#define ILI9341_PASET 0x2B  ///< Page Address Set
#define ILI9341_RAMWR 0x2C  ///< Memory Write
#define ILI9341_RAMRD 0x2E  ///< Memory Read

#define ILI9341_PTLAR 0x30     ///< Partial Area
#define ILI9341_VSCRDEF 0x33   ///< Vertical Scrolling Definition
#define ILI9341_MADCTL 0x36    ///< Memory Access Control
#define ILI9341_VSCRSADD 0x37  ///< Vertical Scrolling Start Address
#define ILI9341_PIXFMT 0x3A    ///< COLMOD: Pixel Format Set

#define ILI9341_FRMCTR1 0xB1  ///< Frame Rate Control (In Normal Mode/Full Colors)
#define ILI9341_FRMCTR2 0xB2  ///< Frame Rate Control (In Idle Mode/8 colors)
#define ILI9341_FRMCTR3 0xB3  ///< Frame Rate control (In Partial Mode/Full Colors)
#define ILI9341_INVCTR 0xB4   ///< Display Inversion Control
#define ILI9341_DFUNCTR 0xB6  ///< Display Function Control

#define ILI9341_PWCTR1 0xC0  ///< Power Control 1
#define ILI9341_PWCTR2 0xC1  ///< Power Control 2
#define ILI9341_PWCTR3 0xC2  ///< Power Control 3
#define ILI9341_PWCTR4 0xC3  ///< Power Control 4
#define ILI9341_PWCTR5 0xC4  ///< Power Control 5
#define ILI9341_VMCTR1 0xC5  ///< VCOM Control 1
#define ILI9341_VMCTR2 0xC7  ///< VCOM Control 2

#define ILI9341_RDID1 0xDA  ///< Read ID 1
#define ILI9341_RDID2 0xDB  ///< Read ID 2
#define ILI9341_RDID3 0xDC  ///< Read ID 3
#define ILI9341_RDID4 0xDD  ///< Read ID 4

#define ILI9341_GMCTRP1 0xE0  ///< Positive Gamma Correction
#define ILI9341_GMCTRN1 0xE1  ///< Negative Gamma Correction

#define MADCTL_MY 0x80   ///< Bottom to top
#define MADCTL_MX 0x40   ///< Right to left
#define MADCTL_MV 0x20   ///< Reverse Mode
#define MADCTL_ML 0x10   ///< LCD refresh Bottom to top
#define MADCTL_RGB 0x00  ///< Red-Green-Blue pixel order
#define MADCTL_BGR 0x08  ///< Blue-Green-Red pixel order
#define MADCTL_MH 0x04   ///< LCD refresh right to left

typedef struct _dgx_ili9341_t {
    dgx_screen_t base;
    dgx_bus_protocols_t *bus;
    gpio_num_t rst;
} dgx_ili9341_t;

static const char TAG[] = "DGX ST 7735";

static uint8_t *dgx_ili9341_adj_madctl(dgx_screen_with_bus_t *scr, const struct _dgx_lcd_init_cmd_t *init_cmd) {
    static uint8_t adj_data[1];
    if (scr->scr.rgb_order == DgxScreenBGR)
        adj_data[0] = init_cmd->data[0] & ~MADCTL_BGR;
    else
        adj_data[0] = init_cmd->data[0] | MADCTL_BGR;
    return adj_data;
}

static uint8_t *dgx_ili9341_adj_colmod(dgx_screen_with_bus_t *scr, const struct _dgx_lcd_init_cmd_t *init_cmd) {
    static uint8_t adj_data[1];
    if (scr->scr.color_bits == 18)
        adj_data[0] = 0x66;
    else
        adj_data[0] = 0x55;  // 16 Bits
    return adj_data;
}

DRAM_ATTR static const dgx_lcd_init_cmd_t  st_init_cmds[]  = {
    //
    {ILI9341_SWRESET, 0, 0, 150, {0}},                       // Soft Reset
    {ILI9341_DISPOFF, 0, 0, 0, {0}},                         // Display Off
    {ILI9341_PIXFMT, dgx_ili9341_adj_colmod, 1, 0, {0x55}},  // Pixel read=565, write=565.
    {ILI9341_PWCTR1, 0, 1, 0, {0x23}},                       //
    {ILI9341_PWCTR2, 0, 1, 0, {0x10}},                       //
    {ILI9341_VMCTR1, 0, 2, 0, {0x3E, 0x28}},                 //
    {ILI9341_VMCTR2, 0, 1, 0, {0x86}},                       //
    {ILI9341_MADCTL, dgx_ili9341_adj_madctl, 1, 0, {0}},     //
    {ILI9341_FRMCTR1, 0, 2, 0, {0x00, 0x18}},                //
    {ILI9341_DFUNCTR, 0, 4, 0, {0x0A, 0xA2, 0x27, 0x04}},    //
    {ILI9341_GAMMASET, 0, 1, 0, {0x01}},                     //
    {ILI9341_GMCTRP1, 0, 15, 0, {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00}},  //
    {ILI9341_GMCTRN1, 0, 15, 0, {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F}},  //
    {ILI9341_SLPOUT, 0, 0, 150, {0}},                                                                                         // Sleep Out
    {ILI9341_DISPON, 0, 0, 0, {0}},                                                                                           // Display On
    {0, 0, 0xff, 0, {0}}                                                                                                      // Stop
};

DRAM_ATTR static const dgx_lcd_init_cmd_t st_init_cmds_tft[]  __attribute__ ((unused)) = {
    //
    {0xEF, 0, 3, 0, {0x03, 0x80, 0x02}},                     // Soft Reset
    {0xCF, 0, 3, 0, {0x00, 0XC1, 0X30}},                     // Display Off
    {0xED, 0, 4, 0, {0x64, 0x03, 0X12, 0X81}},               // Power on sequence control
    {0xE8, 0, 3, 0, {0x85, 0x00, 0x78}},                     // Driver timing control A
    {0xCB, 0, 5, 0, {0x39, 0x2C, 0x00, 0x34, 0x02}},         // Power control A
    {0xF7, 0, 1, 0, {0x20}},                                 // Pump ratio control
    {0xEA, 0, 2, 0, {0x00, 0x00}},                           // Driver timing control B
    {0xEA, 0, 2, 0, {0x00, 0x00}},                           // Driver timing control B
    {ILI9341_PWCTR1, 0, 1, 0, {0x23}},                       //
    {ILI9341_PWCTR2, 0, 1, 0, {0x10}},                       //
    {ILI9341_VMCTR1, 0, 2, 0, {0x3E, 0x28}},                 //
    {ILI9341_VMCTR2, 0, 1, 0, {0x86}},                       //
    {ILI9341_MADCTL, dgx_ili9341_adj_madctl, 1, 0, {0}},     //
    {ILI9341_PIXFMT, dgx_ili9341_adj_colmod, 1, 0, {0x55}},  // Pixel read=565, write=565.
    {ILI9341_FRMCTR1, 0, 2, 0, {0x00, 0x13}},                //
    {ILI9341_DFUNCTR, 0, 4, 0, {0x0A, 0xA2, 0x27, 0x04}},    //
    {0xF2, 0, 1, 0, {0x00}},                                 // Enable 3 gamma
    {ILI9341_GAMMASET, 0, 1, 0, {0x01}},                     //
    {ILI9341_GMCTRP1, 0, 15, 0, {0x0F, 0x2A, 0x28, 0x08, 0x0E, 0x08, 0x54, 0xA9, 0x43, 0x0A, 0x0F, 0x00, 0x00, 0x00, 0x00}},  //
    {ILI9341_GMCTRN1, 0, 15, 0, {0x00, 0x15, 0x17, 0x07, 0x11, 0x06, 0x2B, 0x56, 0x3C, 0x05, 0x10, 0x0F, 0x3F, 0x3F, 0x0F}},  //
    {ILI9341_SLPOUT, 0, 0, 150, {0}},                                                                                         // Sleep Out
    {ILI9341_DISPON, 0, 0, 0, {0}},                                                                                           // Display On
    {0, 0, 0xff, 0, {0}}                                                                                                      // Stop
};

// Initialize the display
dgx_screen_t *dgx_ili9341_init(dgx_bus_protocols_t *bus, gpio_num_t rst, uint8_t color_bits, dgx_color_order_t cbo) {
    // Attach the LCD to the SPI bus
    dgx_ili9341_t *scr = (dgx_ili9341_t *)calloc(1, sizeof(dgx_ili9341_t));
    if (!scr) {
        ESP_LOGE(TAG, "Screen structure memory allocation failed");
    }
    scr->rst = rst;
    scr->bus = bus;
    scr->base.cg_row_shift = 0;
    scr->base.cg_col_shift = 0;
    scr->base.width = 240;
    scr->base.height = 320;
    scr->base.color_bits = color_bits;
    scr->base.rgb_order = cbo;
    scr->base.screen_name = "ILI 9341";
    scr->base.screen_submodel = 0;
    scr->base.screen_subtype = DgxPhysicalScreenWithBus;
    bus->xcmd_set = ILI9341_CASET;   // Column Address Set
    bus->ycmd_set = ILI9341_PASET;   // Row address set
    bus->wcmd_send = ILI9341_RAMWR;  // memory write
    // Memory read is not supported yet
    bus->rcmd_send = 0;  // ILI9341_RAMRD;           //memory read
                         // Initialize non-SPI GPIOs
    dgx_gpio_set_direction(rst, GPIO_MODE_OUTPUT);
    // Reset the display
    dgx_gpio_set_level(rst, 1);
    dgx_delay(100);
    dgx_gpio_set_level(rst, 0);
    dgx_delay(100);
    dgx_gpio_set_level(rst, 1);
    // Send all the commands
    dgx_lcd_init((dgx_screen_with_bus_t *)scr, st_init_cmds);
    dgx_scr_init_slow_bus_optimized_funcs((dgx_screen_t *)scr);
    return (dgx_screen_t *)scr;
}

void dgx_ili9341_orientation(dgx_screen_t *_scr, dgx_orientation_t dir_x, dgx_orientation_t dir_y, bool swap_xy) {
    uint8_t cmd = ILI9341_MADCTL;
    uint8_t data[1] = {0};
    dgx_ili9341_t *scr = (dgx_ili9341_t *)_scr;
    if (dir_x == DgxScreenRightLeft) data[0] |= MADCTL_MX;
    if (dir_y == DgxScreenBottomTop) data[0] |= MADCTL_MY;
    if (swap_xy) {
        data[0] |= MADCTL_MV;
        scr->base.width = 320;
        scr->base.height = 240;
    }
    if (scr->base.rgb_order == DgxScreenRGB) data[0] |= MADCTL_BGR;
    scr->bus->write_command(scr->bus, cmd);
    scr->bus->write_data(scr->bus, data, 1);
}
