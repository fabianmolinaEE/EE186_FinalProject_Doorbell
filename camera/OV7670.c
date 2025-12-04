#include "camera.h"
#include <string.h>
#include <stdio.h>

/* These are defined in main.c, so we declare them here */
extern DCMI_HandleTypeDef hdcmi;
extern I2C_HandleTypeDef  hi2c1;
extern UART_HandleTypeDef hlpuart1;

/* Internal defines */
#define CAM_ADDR        (0x42)
#define OV7670_REG_NUM  45
#define IMG_RDY         1u
#define IMG_NOT_RDY     0u

/* OV7670 register table (unchanged from your main.c) */
static const uint8_t OV7670_reg[OV7670_REG_NUM][2] = {
    { 0x12, 0x80 },
    {0x12, 0x14},   // QVGA, RGB
    {0x8C, 0x00},   // RGB444 Disable
    {0x40, 0x10 + 0xc0},   // RGB565, 00 - FF
    {0x3A, 0x04 + 8},   // UYVY (why?)
    {0x3D, 0x80 + 0x00},   // gamma enable, UV auto adjust, UYVY
    {0xB0, 0x84}, // important

    /* clock related */
    {0x0C, 0x04},  // DCW enable
    {0x3E, 0x19},  // manual scaling, pclk/=2
    {0x70, 0x3A},  // scaling_xsc
    {0x71, 0x35},  // scaling_ysc
    {0x72, 0x11}, // down sample by 2
    {0x73, 0xf1}, // DSP clock /= 2

    /* windowing (empirically decided...) */
    {0x17, 0x14},   // HSTART
    {0x18, 0x02},   // HSTOP
    {0x32, 0x80},   // HREF
    {0x19, 0x02},   // VSTART
    {0x1a, 0x7a},   // VSTOP
    {0x03, 0x0a},   // VREF

    /* color matrix coefficient */
#if 0
    {0x4f, 0xb3},
    {0x50, 0xb3},
    {0x51, 0x00},
    {0x52, 0x3d},
    {0x53, 0xa7},
    {0x54, 0xe4},
    {0x58, 0x9e},
#else
    {0x4f, 0x80},
    {0x50, 0x80},
    {0x51, 0x00},
    {0x52, 0x22},
    {0x53, 0x5e},
    {0x54, 0x80},
    {0x58, 0x9e},
#endif

    /* 3a / AWB, etc (mostly commented out in your original) */
    {0x41, 0x38},   // edge enhancement, de-noise, AWG gain enabled

    /* gamma curve */
#if 1
    {0x7b, 16},
    {0x7c, 30},
    {0x7d, 53},
    {0x7e, 90},
    {0x7f, 105},
    {0x80, 118},
    {0x81, 130},
    {0x82, 140},
    {0x83, 150},
    {0x84, 160},
    {0x85, 180},
    {0x86, 195},
    {0x87, 215},
    {0x88, 230},
    {0x89, 244},
    {0x7a, 16},
#else
    {0x7b, 4},
    {0x7c, 8},
    {0x7d, 16},
    {0x7e, 32},
    {0x7f, 40},
    {0x80, 48},
    {0x81, 56},
    {0x82, 64},
    {0x83, 72},
    {0x84, 80},
    {0x85, 96},
    {0x86, 112},
    {0x87, 144},
    {0x88, 176},
    {0x89, 208},
    {0x7a, 64},
#endif

    /* fps */
    // {0x6B, 0x4a}, // PLL x4
    {0x11, 0x00},   // pre-scalar = 1/1

    /* others */
    {0x1E, 0x31},   // mirror flip
    // {0x42, 0x08}, // color bar
};

/* Image buffer + ready flag (used by DMA + app) */
uint32_t cameraImageBuffer[IMAGE_WORD_LEN];
volatile uint8_t cameraImageReady = IMG_NOT_RDY;

/* --- Camera init + config --- */

void Camera_OV7670_Init(uint8_t debug_mode)
{
    uint8_t ret;
    uint8_t buf_str[32];
    uint8_t write_buf[2];

    for (int i = 0; i < OV7670_REG_NUM; i++) {
        uint8_t reg_addr = OV7670_reg[i][0];
        uint8_t reg_val  = OV7670_reg[i][1];
        write_buf[0] = reg_addr;
        write_buf[1] = reg_val;

        ret = HAL_I2C_Master_Transmit(&hi2c1, CAM_ADDR, write_buf, 2, HAL_MAX_DELAY);
        if (ret != HAL_OK) {
            if (debug_mode == CAMERA_DBG_ON) {
                strcpy((char*)buf_str, "error I2C Cam Config\r\n");
                HAL_UART_Transmit(&hlpuart1, buf_str, strlen((char *)buf_str), 0xFFFF);
            } else {
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
            }
        }
        HAL_Delay(20);
    }
}

void Camera_OV7670_Check(uint8_t debug_mode)
{
    if (debug_mode != CAMERA_DBG_ON) return;

    uint8_t ret;
    uint8_t buf_str[40];

    for (int i = 1; i < OV7670_REG_NUM; i++) {
        uint8_t reg_addr = OV7670_reg[i][0];
        uint8_t read_buf = 0;

        ret = HAL_I2C_Master_Transmit(&hi2c1, CAM_ADDR, &reg_addr, 1, HAL_MAX_DELAY);
        if (ret != HAL_OK) {
            strcpy((char*)buf_str, "error I2C Cam Config Read\r\n");
            HAL_UART_Transmit(&hlpuart1, buf_str, strlen((char *)buf_str), 0xFFFF);
        }

        ret = HAL_I2C_Master_Receive(&hi2c1, CAM_ADDR, &read_buf, 1, HAL_MAX_DELAY);
        if (ret != HAL_OK) {
            strcpy((char*)buf_str, "error I2C Cam Config\r\n");
            HAL_UART_Transmit(&hlpuart1, buf_str, strlen((char *)buf_str), 0xFFFF);
        }

        snprintf((char*)buf_str, sizeof(buf_str),
                 "Register %x with val %x \r\n", reg_addr, read_buf);
        HAL_UART_Transmit(&hlpuart1, buf_str, strlen((char *)buf_str), 0xFFFF);
    }
}

/* --- DCMI frame callback --- */

void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi_arg)
{
    (void)hdcmi_arg;
    cameraImageReady = IMG_RDY;
}

/* --- Capture control --- */

HAL_StatusTypeDef Camera_StartCapture(uint8_t debug_mode)
{
    /* Initialize image buffer with pattern (same as your main) */
    for (int i = 0; i < IMAGE_WORD_LEN; i++) {
        cameraImageBuffer[i] = 0xA5A5A5A5;
    }

    uint8_t buf_str[32];
    HAL_StatusTypeDef ret = HAL_DCMI_Start_DMA(
        &hdcmi,
        DCMI_MODE_SNAPSHOT,
        (uint32_t*)cameraImageBuffer,
        IMAGE_WORD_LEN
    );

    if (debug_mode == CAMERA_DBG_ON) {
        if (ret == HAL_OK) {
            snprintf((char*)buf_str, sizeof(buf_str), "DMA Success HAL_OK\r\n");
            HAL_UART_Transmit(&hlpuart1, buf_str, strlen((char *)buf_str), 0xFFFF);
        } else {
            snprintf((char*)buf_str, sizeof(buf_str), "DMA Start Error\r\n");
            HAL_UART_Transmit(&hlpuart1, buf_str, strlen((char *)buf_str), 0xFFFF);
        }
    }

    if (ret != HAL_OK && debug_mode == CAMERA_DBG_OFF) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
    }

    return ret;
}

/* --- Encapsulated UART transmit logic (the main-loop chunk) --- */

void Camera_TransmitFrame(UART_HandleTypeDef *huart, uint8_t i2c_error)
{
    if (cameraImageReady != IMG_RDY) {
        return; // nothing to do
    }

    uint8_t *image_bytes = (uint8_t *)cameraImageBuffer;
    uint32_t total_bytes = IMAGE_WORD_LEN * sizeof(uint32_t); // 4 bytes/word

    /* This mirrors your 3-chunk transmit logic */

    if (i2c_error == 0) {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
    }
    HAL_UART_Transmit(huart, image_bytes, 0xFFFF, HAL_MAX_DELAY);

    if (i2c_error == 0) {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
    }
    HAL_UART_Transmit(huart, image_bytes + 0xFFFF, 0xFFFF, HAL_MAX_DELAY);

    if (i2c_error == 0) {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
    }
    HAL_UART_Transmit(huart, image_bytes + (0xFFFF * 2),
                      total_bytes - (0xFFFF * 2),
                      HAL_MAX_DELAY);

    cameraImageReady = IMG_NOT_RDY;
}
