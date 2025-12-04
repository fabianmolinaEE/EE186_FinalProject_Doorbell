#ifndef CAMERA_H
#define CAMERA_H

#include "main.h"  // brings in HAL handles, GPIO defs, etc.

/* Image configuration */
#define IM_LEN         320
#define IM_WIDTH       240
#define IMAGE_WORD_LEN ((IM_LEN * IM_WIDTH) / 2)

/* Debug modes */
#define CAMERA_DBG_ON   1u
#define CAMERA_DBG_OFF  0u

/* Public API */
void Camera_OV7670_Init(uint8_t debug_mode);
void Camera_OV7670_Check(uint8_t debug_mode);

/* Start a single-frame capture using DCMI + DMA */
HAL_StatusTypeDef Camera_StartCapture(uint8_t debug_mode);

/* Called repeatedly in main loop – will transmit a frame over UART if ready */
void Camera_TransmitFrame(UART_HandleTypeDef *huart, uint8_t i2c_error);

/* Expose buffer in case you want to inspect it elsewhere */
extern uint32_t cameraImageBuffer[IMAGE_WORD_LEN];

/* Expose “ready” flag if you want to peek at it (optional) */
extern volatile uint8_t cameraImageReady;

#endif /* CAMERA_H */
