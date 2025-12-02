/* oled.c - Minimal SPI display driver for small ST7735-like 80x160 display
 * Note: This implementation is intentionally small and portable. It sets the
 * configured GPIO pins to output mode during init so you don't need to edit
 * `gpio.c`. If you've wired the module to different pins, update the defines
 * in `oled.h` before building.
 */

#include "oled.h"
#include <string.h>

/* Display resolution for the 0.96" 80x160 module */
#define OLED_WIDTH  160
#define OLED_HEIGHT 80

/* Use the provided SPI handle or default hspi1 */
static SPI_HandleTypeDef *oled_hspi = NULL;

/* Small 5x7 font (ASCII 32..127). Each char: 5 bytes columns. */
static const uint8_t font5x7[] = {
// 96 characters * 5 bytes = 480 bytes. Minimal font (space..~)
// Data taken from public-domain 5x7 fonts used commonly in embedded projects
  0x00,0x00,0x00,0x00,0x00, /* ' ' */
  0x00,0x00,0x5F,0x00,0x00, /* ! */
  0x00,0x07,0x00,0x07,0x00, /* " */
  0x14,0x7F,0x14,0x7F,0x14, /* # */
  0x24,0x2A,0x7F,0x2A,0x12, /* $ */
  0x23,0x13,0x08,0x64,0x62, /* % */
  0x36,0x49,0x55,0x22,0x50, /* & */
  0x00,0x05,0x03,0x00,0x00, /* ' */
  0x00,0x1C,0x22,0x41,0x00, /* ( */
  0x00,0x41,0x22,0x1C,0x00, /* ) */
  0x14,0x08,0x3E,0x08,0x14, /* * */
  0x08,0x08,0x3E,0x08,0x08, /* + */
  0x00,0x50,0x30,0x00,0x00, /* , */
  0x08,0x08,0x08,0x08,0x08, /* - */
  0x00,0x60,0x60,0x00,0x00, /* . */
  0x20,0x10,0x08,0x04,0x02, /* / */
  0x3E,0x51,0x49,0x45,0x3E, /* 0 */
  0x00,0x42,0x7F,0x40,0x00, /* 1 */
  0x42,0x61,0x51,0x49,0x46, /* 2 */
  0x21,0x41,0x45,0x4B,0x31, /* 3 */
  0x18,0x14,0x12,0x7F,0x10, /* 4 */
  0x27,0x45,0x45,0x45,0x39, /* 5 */
  0x3C,0x4A,0x49,0x49,0x30, /* 6 */
  0x01,0x71,0x09,0x05,0x03, /* 7 */
  0x36,0x49,0x49,0x49,0x36, /* 8 */
  0x06,0x49,0x49,0x29,0x1E, /* 9 */
  0x00,0x36,0x36,0x00,0x00, /* : */
  0x00,0x56,0x36,0x00,0x00, /* ; */
  0x08,0x14,0x22,0x41,0x00, /* < */
  0x14,0x14,0x14,0x14,0x14, /* = */
  0x00,0x41,0x22,0x14,0x08, /* > */
  0x02,0x01,0x51,0x09,0x06, /* ? */
  0x32,0x49,0x79,0x41,0x3E, /* @ */
  0x7E,0x11,0x11,0x11,0x7E, /* A */
  0x7F,0x49,0x49,0x49,0x36, /* B */
  0x3E,0x41,0x41,0x41,0x22, /* C */
  0x7F,0x41,0x41,0x22,0x1C, /* D */
  0x7F,0x49,0x49,0x49,0x41, /* E */
  0x7F,0x09,0x09,0x09,0x01, /* F */
  0x3E,0x41,0x49,0x49,0x7A, /* G */
  0x7F,0x08,0x08,0x08,0x7F, /* H */
  0x00,0x41,0x7F,0x41,0x00, /* I */
  0x20,0x40,0x41,0x3F,0x01, /* J */
  0x7F,0x08,0x14,0x22,0x41, /* K */
  0x7F,0x40,0x40,0x40,0x40, /* L */
  0x7F,0x02,0x0C,0x02,0x7F, /* M */
  0x7F,0x04,0x08,0x10,0x7F, /* N */
  0x3E,0x41,0x41,0x41,0x3E, /* O */
  0x7F,0x09,0x09,0x09,0x06, /* P */
  0x3E,0x41,0x51,0x21,0x5E, /* Q */
  0x7F,0x09,0x19,0x29,0x46, /* R */
  0x46,0x49,0x49,0x49,0x31, /* S */
  0x01,0x01,0x7F,0x01,0x01, /* T */
  0x3F,0x40,0x40,0x40,0x3F, /* U */
  0x1F,0x20,0x40,0x20,0x1F, /* V */
  0x3F,0x40,0x38,0x40,0x3F, /* W */
  0x63,0x14,0x08,0x14,0x63, /* X */
  0x07,0x08,0x70,0x08,0x07, /* Y */
  0x61,0x51,0x49,0x45,0x43, /* Z */
  0x00,0x7F,0x41,0x41,0x00, /* [ */
  0x02,0x04,0x08,0x10,0x20, /* \ */
  0x00,0x41,0x41,0x7F,0x00, /* ] */
  0x04,0x02,0x01,0x02,0x04, /* ^ */
  0x40,0x40,0x40,0x40,0x40, /* _ */
  0x00,0x01,0x02,0x04,0x00, /* ` */
  0x20,0x54,0x54,0x54,0x78, /* a */
  0x7F,0x48,0x44,0x44,0x38, /* b */
  0x38,0x44,0x44,0x44,0x20, /* c */
  0x38,0x44,0x44,0x48,0x7F, /* d */
  0x38,0x54,0x54,0x54,0x18, /* e */
  0x08,0x7E,0x09,0x01,0x02, /* f */
  0x0C,0x52,0x52,0x52,0x3E, /* g */
  0x7F,0x08,0x04,0x04,0x78, /* h */
  0x00,0x44,0x7D,0x40,0x00, /* i */
  0x20,0x40,0x44,0x3D,0x00, /* j */
  0x7F,0x10,0x28,0x44,0x00, /* k */
  0x00,0x41,0x7F,0x40,0x00, /* l */
  0x7C,0x04,0x18,0x04,0x78, /* m */
  0x7C,0x08,0x04,0x04,0x78, /* n */
  0x38,0x44,0x44,0x44,0x38, /* o */
  0x7C,0x14,0x14,0x14,0x08, /* p */
  0x08,0x14,0x14,0x18,0x7C, /* q */
  0x7C,0x08,0x04,0x04,0x08, /* r */
  0x48,0x54,0x54,0x54,0x20, /* s */
  0x04,0x3F,0x44,0x40,0x20, /* t */
  0x3C,0x40,0x40,0x20,0x7C, /* u */
  0x1C,0x20,0x40,0x20,0x1C, /* v */
  0x3C,0x40,0x30,0x40,0x3C, /* w */
  0x44,0x28,0x10,0x28,0x44, /* x */
  0x0C,0x50,0x50,0x50,0x3C, /* y */
  0x44,0x64,0x54,0x4C,0x44, /* z */
  0x00,0x08,0x36,0x41,0x00, /* { */
  0x00,0x00,0x7F,0x00,0x00, /* | */
  0x00,0x41,0x36,0x08,0x00, /* } */
  0x02,0x01,0x02,0x04,0x02, /* ~ (approx) */
};

/* Low-level helpers */
static void oled_cs_low(void){ HAL_GPIO_WritePin(OLED_CS_GPIO_Port, OLED_CS_Pin, GPIO_PIN_RESET); }
static void oled_cs_high(void){ HAL_GPIO_WritePin(OLED_CS_GPIO_Port, OLED_CS_Pin, GPIO_PIN_SET); }
static void oled_dc_cmd(void){ HAL_GPIO_WritePin(OLED_DC_GPIO_Port, OLED_DC_Pin, GPIO_PIN_RESET); }
static void oled_dc_data(void){ HAL_GPIO_WritePin(OLED_DC_GPIO_Port, OLED_DC_Pin, GPIO_PIN_SET); }
static void oled_reset_low(void){ HAL_GPIO_WritePin(OLED_RST_GPIO_Port, OLED_RST_Pin, GPIO_PIN_RESET); }
static void oled_reset_high(void){ HAL_GPIO_WritePin(OLED_RST_GPIO_Port, OLED_RST_Pin, GPIO_PIN_SET); }

static void oled_gpio_init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Ensure port clocks enabled */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /* Configure CS (PA4) as push-pull output */
  GPIO_InitStruct.Pin = OLED_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(OLED_CS_GPIO_Port, &GPIO_InitStruct);

  /* Configure DC (PD9) and RST (PD8) as push-pull outputs */
  GPIO_InitStruct.Pin = OLED_DC_Pin | OLED_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(OLED_DC_GPIO_Port, &GPIO_InitStruct);

  oled_cs_high();
  oled_dc_data();
  oled_reset_high();
}

static void oled_send_cmd(uint8_t cmd)
{
  oled_dc_cmd();
  oled_cs_low();
  HAL_SPI_Transmit(oled_hspi, &cmd, 1, HAL_MAX_DELAY);
  oled_cs_high();
}

static void oled_send_data(uint8_t *data, uint16_t len)
{
  oled_dc_data();
  oled_cs_low();
  HAL_SPI_Transmit(oled_hspi, data, len, HAL_MAX_DELAY);
  oled_cs_high();
}

/* Set column and row window (inclusive) - with bounds checking */
static void oled_set_window(uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1)
{
  /* Clamp to display bounds to prevent accessing hidden VRAM */
  if(x0 > OLED_WIDTH-1) x0 = OLED_WIDTH-1;
  if(x1 > OLED_WIDTH-1) x1 = OLED_WIDTH-1;
  if(y0 > OLED_HEIGHT-1) y0 = OLED_HEIGHT-1;
  if(y1 > OLED_HEIGHT-1) y1 = OLED_HEIGHT-1;
  
  uint8_t data[4];
  oled_send_cmd(0x2A); /* CASET - columns */
  data[0]=(uint8_t)((x0>>8)&0xFF); data[1]=(uint8_t)(x0&0xFF); 
  data[2]=(uint8_t)((x1>>8)&0xFF); data[3]=(uint8_t)(x1&0xFF);
  oled_send_data(data,4);
  oled_send_cmd(0x2B); /* RASET - rows */
  data[0]=(uint8_t)((y0>>8)&0xFF); data[1]=(uint8_t)(y0&0xFF); 
  data[2]=(uint8_t)((y1>>8)&0xFF); data[3]=(uint8_t)(y1&0xFF);
  oled_send_data(data,4);
  oled_send_cmd(0x2C); /* RAMWR */
}

void OLED_Init(SPI_HandleTypeDef *hspi)
{
  if(hspi==NULL) oled_hspi = &hspi1;
  else oled_hspi = hspi;

  oled_gpio_init();

  /* Hardware reset - critical for reliable init */
  oled_reset_low();
  HAL_Delay(10);
  oled_reset_high();
  HAL_Delay(120);

  /* Simplified ST7735 init sequence for 80x160 display */
  oled_send_cmd(0x01); /* Software reset */
  HAL_Delay(150);
  
  oled_send_cmd(0x11); /* Sleep out */
  HAL_Delay(100);
  
  /* Set color mode to 16-bit/pixel (0x05 = RGB565) */
  oled_send_cmd(0x3A);
  uint8_t colmod = 0x05;
  oled_send_data(&colmod, 1);
  HAL_Delay(10);
  
  /* Memory Data Access Control - rotation (try different values for correct orientation) */
  oled_send_cmd(0x36);
  uint8_t madctl = 0xA0; /* Try 0xA0, 0x00, 0x60, 0xC0 if this doesn't work */
  oled_send_data(&madctl, 1);
  HAL_Delay(10);

  /* Set row address offset to center the 80-pixel display in 160 rows */
  oled_send_cmd(0x37); /* VSCSAD - Vertical Scroll Start Address */
  uint8_t vscsad[] = {0x00, 0x28}; /* Start at row 40 (160-80)/2 = 40, but try 0x00 first */
  oled_send_data(vscsad, 2);
  HAL_Delay(10);

  /* Inversion ON (common for this type of display) */
  oled_send_cmd(0x21);
  HAL_Delay(10);

  /* Column/Page address set (window coordinates for 160x80 after rotation) */
  oled_send_cmd(0x2A);
  uint8_t caset[] = {0x00, 0x00, 0x00, 0x9F}; /* 0..159 columns */
  oled_send_data(caset, 4);
  oled_send_cmd(0x2B);
  uint8_t raset[] = {0x00, 0x00, 0x00, 0x4F}; /* 0..79 rows */
  oled_send_data(raset, 4);

  /* Frame rate control */
  oled_send_cmd(0xB1);
  uint8_t frate[] = {0x05, 0x3C, 0x3C};
  oled_send_data(frate, 3);

  /* Display ON */
  oled_send_cmd(0x29);
  HAL_Delay(50);

  /* Clear entire display RAM (both visible and hidden areas) to prevent artifacts */
  oled_send_cmd(0x2A);
  uint8_t caset_full[] = {0x00, 0x00, 0x00, 0x9F}; /* Full column range */
  oled_send_data(caset_full, 4);
  oled_send_cmd(0x2B);
  uint8_t raset_full[] = {0x00, 0x00, 0x00, 0x9F}; /* Full row range (160 rows) */
  oled_send_data(raset_full, 4);
  oled_send_cmd(0x2C);
  
  /* Send black pixels for entire 160x160 area */
  uint8_t black[2] = {0x00, 0x00};
  for(uint32_t i = 0; i < 160*160; i++){
    oled_dc_data(); oled_cs_low();
    HAL_SPI_Transmit(oled_hspi, black, 2, HAL_MAX_DELAY);
    oled_cs_high();
  }
  
  HAL_Delay(50);

  /* Reset window to actual display area */
  oled_send_cmd(0x2A);
  oled_send_data(caset, 4);
  oled_send_cmd(0x2B);
  oled_send_data(raset, 4);

  OLED_FillScreen(RGB565(0,0,0));
  HAL_Delay(100);
}

void OLED_FillScreen(color565_t color)
{
  /* Explicitly set window to full display area */
  oled_send_cmd(0x2A); /* CASET */
  uint8_t caset[] = {0x00, 0x00, 0x00, (OLED_WIDTH-1)};
  oled_send_data(caset, 4);
  oled_send_cmd(0x2B); /* RASET */
  uint8_t raset[] = {0x00, 0x00, 0x00, (OLED_HEIGHT-1)};
  oled_send_data(raset, 4);
  oled_send_cmd(0x2C); /* RAMWR */

  /* Prepare two-byte color */
  uint8_t data[2];
  data[0] = (uint8_t)(color>>8);
  data[1] = (uint8_t)(color&0xFF);

  /* Send large buffer in chunks to avoid huge RAM usage */
  const uint32_t pixels = (uint32_t)OLED_WIDTH * OLED_HEIGHT;
  const uint16_t chunkPixels = 128;
  uint8_t chunk[chunkPixels*2];
  for(uint16_t i=0;i<chunkPixels;i++){ chunk[2*i]=data[0]; chunk[2*i+1]=data[1]; }

  uint32_t remaining = pixels;
  while(remaining){
    uint32_t toSend = (remaining>chunkPixels)?chunkPixels:remaining;
    oled_dc_data(); oled_cs_low();
    HAL_SPI_Transmit(oled_hspi, chunk, toSend*2, HAL_MAX_DELAY);
    oled_cs_high();
    remaining -= toSend;
  }
}

/* Draw a pixel (slow) - used by text rendering */
static void oled_draw_pixel(uint16_t x, uint16_t y, color565_t color)
{
  if(x>=OLED_WIDTH || y>=OLED_HEIGHT) return;
  oled_set_window(x,y,x,y);
  uint8_t col[2] = {(uint8_t)(color>>8),(uint8_t)(color&0xFF)};
  oled_send_data(col,2);
}

void OLED_DrawString(uint16_t x, uint16_t y, const char *s, color565_t color, color565_t bgcolor)
{
  /* Ensure we're in bounds and only draw within the active display area */
  if(y >= OLED_HEIGHT) return;
  
  while(*s){
    char c = *s++;
    if(c<32 || c>127) c='?';
    const uint8_t *ch = &font5x7[(c-32)*5];
    for(uint8_t col=0; col<5; col++){
      uint8_t bits = ch[col];
      for(uint8_t row=0; row<8; row++){
        uint16_t px = x + col;
        uint16_t py = y + row;
        /* Strict bounds checking - do not draw outside active area */
        if(px < OLED_WIDTH && py < OLED_HEIGHT){
          if(bits & (1<<row)) oled_draw_pixel(px, py, color);
          else oled_draw_pixel(px, py, bgcolor);
        }
      }
    }
    /* one-column gap */
    for(uint8_t row=0; row<8; row++){
      uint16_t px = x + 5;
      uint16_t py = y + row;
      if(px < OLED_WIDTH && py < OLED_HEIGHT){
        oled_draw_pixel(px, py, bgcolor);
      }
    }
    x += 6;
    if(x >= OLED_WIDTH) break;
  }
}

void OLED_ShowState(SystemState_t st)
{
  OLED_FillScreen(RGB565(0,0,0));
  switch(st){
    case SYS_ADD_NEW_FINGERPRINT:
      OLED_DrawString(2, 10, "Add New Finger", RGB565(255,165,0), RGB565(0,0,0));
      OLED_DrawString(2, 30, "Place finger...", RGB565(255,255,255), RGB565(0,0,0));
      break;
    case SYS_WAIT_FOR_SCAN:
      OLED_DrawString(2, 10, "Waiting for", RGB565(0,191,255), RGB565(0,0,0));
      OLED_DrawString(2, 30, "Scan fingerprint", RGB565(0,255,0), RGB565(0,0,0));
      break;
    case SYS_SCAN_WRONG:
      OLED_DrawString(2, 10, "Access Denied", RGB565(255,0,0), RGB565(0,0,0));
      break;
    case SYS_SCAN_CORRECT:
      OLED_DrawString(2, 10, "Access Granted", RGB565(0,255,0), RGB565(0,0,0));
      break;
  }
}

void OLED_Animation_Pulse(color565_t color, uint8_t phase)
{
  /* Simple pulse: draw a small centered filled rectangle with size depending on phase */
  uint8_t size = (phase % 10);
  uint16_t w = 10 + size*2;
  uint16_t h = 8 + size;
  if(w > OLED_WIDTH) w = OLED_WIDTH;
  if(h > OLED_HEIGHT) h = OLED_HEIGHT;
  uint16_t x = (OLED_WIDTH > w) ? (OLED_WIDTH - w)/2 : 0;
  uint16_t y = (OLED_HEIGHT > h) ? (OLED_HEIGHT - h)/2 : 0;
  /* fill small area */
  for(uint16_t yy=0; yy<h && y+yy<OLED_HEIGHT; yy++){
    for(uint16_t xx=0; xx<w && x+xx<OLED_WIDTH; xx++){
      oled_draw_pixel(x+xx, y+yy, color);
    }
  }
}
