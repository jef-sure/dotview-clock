execute_process(COMMAND ${PYTHON} "${COMPONENT_DIR}/src/fontscan.py")

set(srcs "src/drivers/dgx_lcd_init.c" "src/dgx_bus_optimized_funcs.c" "src/draw.c" "src/font.c" "src/orientation.c" "src/bitmap.c")

if(CONFIG_DGX_ENABLE_SPI)
	list(APPEND srcs "src/bus/spi_esp32.c")
endif()

if(CONFIG_DGX_ENABLE_I2C)
	list(APPEND srcs "src/bus/i2c_esp32.c")
endif()

if(CONFIG_DGX_ENABLE_P8)
	list(APPEND srcs "src/bus/p8_esp32.c")
endif()

if(CONFIG_DGX_ENABLE_SPI_ST7735)
	list(APPEND srcs "src/drivers/st7735.c")
endif()

if(CONFIG_DGX_ENABLE_SPI_ST7789)
	list(APPEND srcs "src/drivers/st7789.c")
endif()

if(CONFIG_DGX_ENABLE_SPI_ILI_9341)
	list(APPEND srcs "src/drivers/ili9341.c")
endif()

if(CONFIG_DGX_ENABLE_SSD1351)
	list(APPEND srcs "src/drivers/ssd1351.c")
endif()


if(CONFIG_DGX_ENABLE_VSCREEN)
	list(APPEND srcs "src/drivers/vscreen.c")
endif()

if(CONFIG_DGX_ENABLE_VSCREEN_2H)
	list(APPEND srcs "src/drivers/vscreen_2h.c")
endif()

file(STRINGS "${COMPONENT_DIR}/src/fonts/fonts.txt" fonts)

if(fonts)
	list(APPEND srcs ${fonts})
endif()

idf_component_register(SRCS "${srcs}" REQUIRES driver INCLUDE_DIRS "include")
#target_compile_options(${COMPONENT_LIB} PRIVATE -Ofast)
