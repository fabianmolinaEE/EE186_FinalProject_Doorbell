/* oled.h - Simple SPI OLED/TFT driver (ST7735-like) for NUCLEO-L4R5ZI
 *
 * Default pins (can be changed to match your wiring):
 *  - CS  : PB1
 *  - DC  : PB2
 *  - RST : PB6
 *
 * The driver uses `hspi1` by default; pass &hspi1 to `OLED_Init` or keep NULL
 * to use the global `hspi1` defined by CubeMX.
 */
#ifndef __OLED_H
#define __OLED_H

#include "main.h"
#include "spi.h"

/* Change these if you connected the module to different pins */
#ifndef OLED_CS_GPIO_Port
#define OLED_CS_GPIO_Port GPIOA
#endif
#ifndef OLED_CS_Pin
#define OLED_CS_Pin GPIO_PIN_4
#endif

#ifndef OLED_DC_GPIO_Port
#define OLED_DC_GPIO_Port GPIOD
#endif
#ifndef OLED_DC_Pin
#define OLED_DC_Pin GPIO_PIN_9
#endif

#ifndef OLED_RST_GPIO_Port
#define OLED_RST_GPIO_Port GPIOD
#endif
#ifndef OLED_RST_Pin
#define OLED_RST_Pin GPIO_PIN_8
#endif

/* Colors in RGB565 */
typedef uint16_t color565_t;
#define RGB565(r,g,b) (uint16_t)((((r)&0xF8)<<8) | (((g)&0xFC)<<3) | (((b)&0xF8)>>3))

/* System states to display */
typedef enum {
  SYS_ADD_NEW_FINGERPRINT = 0,
  SYS_WAIT_FOR_SCAN,
  SYS_SCAN_WRONG,
  SYS_SCAN_CORRECT
} SystemState_t;

/* Initialize display. Pass SPI handle or NULL to use global hspi1. */
void OLED_Init(SPI_HandleTypeDef *hspi);

/* Fill the screen with a color */
void OLED_FillScreen(color565_t color);

/* Draw a string at x,y (top-left) using a small built-in font */
void OLED_DrawString(uint16_t x, uint16_t y, const char *s, color565_t color, color565_t bgcolor);

/* Convenience: show a message based on system state */
void OLED_ShowState(SystemState_t st);

/* Draw a simple animation frame (user may call periodically) */
void OLED_Animation_Pulse(color565_t color, uint8_t phase);

#endif
