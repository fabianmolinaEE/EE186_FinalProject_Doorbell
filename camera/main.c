/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define CAM_ADDR (0x42)
#define OV7670_REG_NUM 45
#define IM_LEN   320
#define IM_WIDTH 240
#define IMAGE_WORD_LEN ((IM_LEN * IM_WIDTH) / 2)
#define IMG_RDY 1
#define IMG_NOT_RDY 0
#define DBG_ON 1
#define DBG_OFF 0
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
DCMI_HandleTypeDef hdcmi;
DMA_HandleTypeDef hdma_dcmi;

I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef hlpuart1;

/* USER CODE BEGIN PV */
const uint8_t OV7670_reg[OV7670_REG_NUM][2] = { { 0x12, 0x80 },
		/*
		{0x11, 0x01}, // Disable PLL, PCLK runs 0.5x Input (HSI16)
		{0x12, 0x14}, // Data Format Register, output format is RGB and QVGA
		{0x40, 0xD0}, // Full data range, use RGB 565 data format for 2 pix per byte

		{0x0C, 0x04}, // COM3 -> DCW Enable (Bit[2])
		{0x3E, 0x1A}, // DCW and scaling PCLK enable, manual scaling enable for resolution modes, pclk divider by 4
		{0x70, 0x3A}, // X Scale
		{0x71, 0x35}, // Y Scale, bit[7] turns on color bar test
		{0x72, 0x11}, // Scale DCWCTR
		{0x73, 0xF9}, // PClk divider
		{0xA2, 0x02}, // Scaling PCLK_Delay
		{0x15, 0x00}
		*/
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
		  {0x19, 0x02},   // VSTART =  10 ( = 2 * 4 + 2)
		  {0x1a, 0x7a},   // VSTOP  = 490 ( = 122 * 4 + 2)
		  {0x03, 0x0a},   // VREF (VSTART_LOW = 2, VSTOP_LOW = 2)
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

		  		  /* 3a */
		  		//  {0x13, 0x84},
		  		//  {0x14, 0x0a},   // AGC Ceiling = 2x
		  		//  {0x5F, 0x2f},   // AWB B Gain Range (empirically decided)
		  		//                  // without this bright scene becomes yellow (purple). might be because of color matrix
		  		//  {0x60, 0x98},   // AWB R Gain Range (empirically decided)
		  		//  {0x61, 0x70},   // AWB G Gain Range (empirically decided)
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
		  		  /* gamma = 1 */
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
		  		//  {0x6B, 0x4a}, //PLL  x4
		  		  {0x11, 0x00}, // pre-scalar = 1/1

		  		  /* others */
		  		  {0x1E, 0x31}, //mirror flip
		  		//  {0x42, 0x08}, // color bar

		  	};

uint8_t image_rdy_flag = IMG_NOT_RDY;
uint32_t image[IMAGE_WORD_LEN];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_DCMI_Init(void);
static void MX_I2C1_Init(void);
static void MX_LPUART1_UART_Init(void);
/* USER CODE BEGIN PFP */
void Camera_7670_Init(uint8_t debug_mode);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void Camera_7670_Init(uint8_t debug_mode){
	uint8_t ret;
	uint8_t buf_str[15];
	uint8_t write_buf[2];
	// loop through registers and desired values in 2D array and write them
	for (int i = 0; i < OV7670_REG_NUM; i++){
		uint8_t reg_addr = OV7670_reg[i][0];
		uint8_t reg_val =  OV7670_reg[i][1];
		write_buf[0] = reg_addr;
		write_buf[1] = reg_val;

		ret = HAL_I2C_Master_Transmit(&hi2c1, CAM_ADDR, write_buf, 2, HAL_MAX_DELAY);
		// Virtual COM to send error messages
		if (ret != HAL_OK && debug_mode == DBG_ON){
			strcpy((char*)buf_str, "error I2C Cam Config\r\n");
			ret = HAL_UART_Transmit(&hlpuart1, buf_str, strlen((char *)buf_str), 0xFFFF);
		}
		else if (ret!= HAL_OK && debug_mode == DBG_OFF){
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
		}
		HAL_Delay(20);
		/*
		ret = HAL_I2C_Master_Transmit(&hi2c1, CAM_ADDR, &reg_val, 1, HAL_MAX_DELAY);
		if (ret != HAL_OK){
			strcpy((char*)buf_str, "error I2C Cam Config\r\n");
			ret = HAL_UART_Transmit(&hlpuart1, buf_str, strlen((char *)buf_str), 0xFFFF);
		}
		*/
	}
}

void Camera_7670_Check(){
	uint8_t ret;
	uint8_t buf_str[30];
	// loop through registers and desired values in 2D array and read them
	for (int i = 1; i < OV7670_REG_NUM; i++){
		uint8_t reg_addr = OV7670_reg[i][0];
		uint8_t read_buf[1];

		ret = HAL_I2C_Master_Transmit(&hi2c1, CAM_ADDR, &reg_addr, 1, HAL_MAX_DELAY);
		// Virtual COM to send error messages
		if (ret != HAL_OK){
			strcpy((char*)buf_str, "error I2C Cam Config Read\r\n");
			ret = HAL_UART_Transmit(&hlpuart1, buf_str, strlen((char *)buf_str), 0xFFFF);
		}
		ret = HAL_I2C_Master_Receive(&hi2c1, CAM_ADDR, read_buf, 1, HAL_MAX_DELAY);

		if (ret != HAL_OK){
			strcpy((char*)buf_str, "error I2C Cam Config\r\n");
			ret = HAL_UART_Transmit(&hlpuart1, buf_str, strlen((char *)buf_str), 0xFFFF);
		}
		// Print values from registers
		snprintf(buf_str, sizeof(buf_str), "Register %x with val %x \r\n", reg_addr, read_buf[0]);
		ret = HAL_UART_Transmit(&hlpuart1, buf_str, strlen((char *)buf_str), 0xFFFF);
	}
}




void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi)
{
	image_rdy_flag = IMG_RDY;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

	// Image Resolution is 352x288
	// RGB 565 -> 2 bytes per pix, 2pix per word
	// (352 * 288) * /2 = 19200 words
	// 19200 words
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_DCMI_Init();
  MX_I2C1_Init();
  MX_LPUART1_UART_Init();
  /* USER CODE BEGIN 2 */
  uint8_t read_buf[1];
  uint8_t id_reg = 0x0A;
  uint8_t buf_str[25];
  uint8_t debug_mode = DBG_OFF;
  uint8_t talk = 0;

  //snprintf(buf_str, sizeof(buf_str), "id bits: 0x%x\r\n", read_buf[0]);
  //HAL_UART_Transmit(&hlpuart1, buf_str, strlen((char *)buf_str), 0xFFFF);

  Camera_7670_Init(debug_mode);


  if (debug_mode == DBG_ON){
	  Camera_7670_Check();
  }
  HAL_I2C_Master_Transmit(&hi2c1, CAM_ADDR, &id_reg, 1, HAL_MAX_DELAY);
  HAL_I2C_Master_Receive(&hi2c1, CAM_ADDR, read_buf, 1, HAL_MAX_DELAY);

  uint8_t i2c_error = 0;
  if (read_buf[0] != 0x76){
	  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
	  i2c_error = 1;
  }
  for (int i = 0; i < IMAGE_WORD_LEN; i++) {
      image[i] = 0xA5A5A5A5;
  }

  uint8_t ret = HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_SNAPSHOT, image, IMAGE_WORD_LEN);
  if (ret == HAL_OK && debug_mode == DBG_ON){
	  snprintf(buf_str, sizeof(buf_str), "DMA Success HAL_OK\r\n");
	  HAL_UART_Transmit(&hlpuart1, buf_str, strlen((char *)buf_str), 0xFFFF);
  }
 //\ HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);

  if (ret != HAL_OK && debug_mode == DBG_OFF){
	  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  // CIF Image Resolution is (340 * 240), takes 2 bytes to represent a pixel so 153600 bytes for image
	  // Max UART transmit 0xFFFF (65535) bytes.
	  // (153600 / 65535) = 2.34 UART transmits

	 if (image_rdy_flag == IMG_RDY){
		 	 if (i2c_error == 0){
		 		 HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
		 	 }
	  		  HAL_UART_Transmit(&hlpuart1, (uint8_t *)image, 0xFFFF, HAL_MAX_DELAY);
		  		if (i2c_error == 0){
				 HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
			 }
	  		  HAL_UART_Transmit(&hlpuart1, (uint8_t *)image + 0xFFFF,  0xFFFF, HAL_MAX_DELAY);
	  		if (i2c_error == 0){
				 HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
			 }
	  		  HAL_UART_Transmit(&hlpuart1, (uint8_t *)image + (0xFFFF * 2), ((IMAGE_WORD_LEN * 4) - (0xFFFF * 2)), HAL_MAX_DELAY);
	  		  image_rdy_flag = IMG_NOT_RDY;
	  	}
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_11;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 20;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_HSI, RCC_MCODIV_1);
}

/**
  * @brief DCMI Initialization Function
  * @param None
  * @retval None
  */
static void MX_DCMI_Init(void)
{

  /* USER CODE BEGIN DCMI_Init 0 */

  /* USER CODE END DCMI_Init 0 */

  /* USER CODE BEGIN DCMI_Init 1 */

  /* USER CODE END DCMI_Init 1 */
  hdcmi.Instance = DCMI;
  hdcmi.Init.SynchroMode = DCMI_SYNCHRO_HARDWARE;
  hdcmi.Init.PCKPolarity = DCMI_PCKPOLARITY_RISING;
  hdcmi.Init.VSPolarity = DCMI_VSPOLARITY_HIGH;
  hdcmi.Init.HSPolarity = DCMI_HSPOLARITY_LOW;
  hdcmi.Init.CaptureRate = DCMI_CR_ALL_FRAME;
  hdcmi.Init.ExtendedDataMode = DCMI_EXTEND_DATA_8B;
  hdcmi.Init.JPEGMode = DCMI_JPEG_DISABLE;
  hdcmi.Init.ByteSelectMode = DCMI_BSM_ALL;
  hdcmi.Init.ByteSelectStart = DCMI_OEBS_ODD;
  hdcmi.Init.LineSelectMode = DCMI_LSM_ALL;
  hdcmi.Init.LineSelectStart = DCMI_OELS_ODD;
  if (HAL_DCMI_Init(&hdcmi) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DCMI_Init 2 */

  /* USER CODE END DCMI_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00503D58;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief LPUART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_LPUART1_UART_Init(void)
{

  /* USER CODE BEGIN LPUART1_Init 0 */

  /* USER CODE END LPUART1_Init 0 */

  /* USER CODE BEGIN LPUART1_Init 1 */

  /* USER CODE END LPUART1_Init 1 */
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 9600;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  hlpuart1.FifoMode = UART_FIFOMODE_DISABLE;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LPUART1_Init 2 */

  /* USER CODE END LPUART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMAMUX1_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  HAL_PWREx_EnableVddIO2();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);

  /*Configure GPIO pins : PE2 PE3 */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF13_SAI1;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PF0 PF1 PF2 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C2;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pin : PF7 */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF13_SAI1;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pin : PA7 */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PC4 PC5 */
  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PB0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PB1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PB2 */
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PE7 PE8 PE9 PE10
                           PE11 PE12 PE13 */
  GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10
                          |GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PE14 PE15 */
  GPIO_InitStruct.Pin = GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF3_TIM1_COMP1;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : PB10 */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB12 PB13 PB15 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF13_SAI2;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PB14 */
  GPIO_InitStruct.Pin = GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PD8 PD9 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : PD14 PD15 */
  GPIO_InitStruct.Pin = GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF2_TIM4;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : PA8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF0_MCO;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PC10 PC12 */
  GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_SDMMC1;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PD0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : PD2 */
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_SDMMC1;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : PD3 PD4 PD5 PD6 */
  GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : PB3 PB4 PB5 */
  GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PE0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF2_TIM4;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
