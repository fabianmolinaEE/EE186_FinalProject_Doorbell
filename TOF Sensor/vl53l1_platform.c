/**
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
#include "i2c.h"
#include "platform/vl53l1_platform.h"
#include <string.h>
#include <time.h>
#include <math.h>

int8_t VL53L1_WriteMulti( uint16_t dev, uint16_t index, uint8_t *pdata, uint32_t count) {
	 if (HAL_I2C_Mem_Write(&hi2c2, dev, index, I2C_MEMADD_SIZE_16BIT, pdata, count, HAL_MAX_DELAY) == HAL_OK) {
		 return 0;
	 }
	 return 1;
}

int8_t VL53L1_ReadMulti(uint16_t dev, uint16_t index, uint8_t *pdata, uint32_t count) {
	if (HAL_I2C_Mem_Read(&hi2c2, dev, index, I2C_MEMADD_SIZE_16BIT, pdata, count, HAL_MAX_DELAY) == HAL_OK) {
		return 0;
	}
	return 1;
}

int8_t VL53L1_WrByte(uint16_t dev, uint16_t index, uint8_t data) {
	return VL53L1_WriteMulti(dev, index, &data, 1);
}

int8_t VL53L1_WrWord(uint16_t dev, uint16_t index, uint16_t data) {
	uint8_t dataBuf[2];
	dataBuf[1] = data & 0b11111111;
	dataBuf[0] = data >> 8;
	return VL53L1_WriteMulti(dev, index, dataBuf, 2);
}

int8_t VL53L1_WrDWord(uint16_t dev, uint16_t index, uint32_t data) {
	uint8_t dataBuf[4];
	dataBuf[3] = data & 0b11111111;
	dataBuf[2] = (data >> 8) & 0b11111111;
	dataBuf[1] = (data >> 16) & 0b11111111;
	dataBuf[0] = (data >> 24) & 0b11111111;
	return VL53L1_WriteMulti(dev, index, dataBuf, 4);
}

int8_t VL53L1_RdByte(uint16_t dev, uint16_t index, uint8_t *data) {
	return VL53L1_ReadMulti(dev, index, data, 1);
}

int8_t VL53L1_RdWord(uint16_t dev, uint16_t index, uint16_t *data) {
	uint8_t dataBuf[2];
	if (VL53L1_ReadMulti(dev, index, dataBuf, 2) == 0) {
		*data = ((uint16_t)dataBuf[0] << 8);
		*data |= dataBuf[1];
		return 0;
	}
	return 1;
}

int8_t VL53L1_RdDWord(uint16_t dev, uint16_t index, uint32_t *data) {
	uint8_t dataBuf[4];
	if(VL53L1_ReadMulti(dev, index, dataBuf, 4) == 0) {
		*data = ((uint32_t)dataBuf[0] << 24);
		*data |= (uint32_t)dataBuf[1] << 16;
		*data |= (uint32_t)dataBuf[2] << 8;
		*data |= (uint32_t)dataBuf[3];
		return 0;
	}
	return 1;
}

int8_t VL53L1_WaitMs(uint16_t dev, int32_t wait_ms){
	HAL_Delay(wait_ms);
	return 0;
}
