#include "main.h"
#include <math.h>
#include <limits.h>

#define MPU9250_Address 0x68
#define TEMP_OUT_H_Address 0x41
#define TEMP_OUT_L_Address 0x42
#define WHO_AM_I_Address 0x75
#define ACCEL_CONFIG_Address 0x1c
#define ACCEL_CONFIG2_Address 0x1d
#define ACCEL_XOUT_H_Address 0x3b
#define ACCEL_XOUT_L_Address 0x3c
#define ACCEL_YOUT_H_Address 0x3d
#define ACCEL_YOUT_L_Address 0x3e
#define ACCEL_ZOUT_H_Address 0x3f
#define ACCEL_ZOUT_L_Address 0x40
#define LP_ACCEL_ODR_Address 0x1e
#define PWR_MGMT_1_Address 0x6b
#define ACC_RESOLUTION 2.0

UART_HandleTypeDef huart1;
I2C_HandleTypeDef hi2c1;

float TEMP_degC;
float FinalTEMP_degC;
float Accel_X_Result;
float Accel_Y_Result;
float Accel_Z_Result;

int AR_Int;
float AR_Float;
int AR_Int2;
char *AR_Sign;

uint16_t rawTemp;
uint8_t readReg;
uint8_t isRegWorking;
uint8_t buffer;

uint16_t Accel_Xout;
uint16_t Accel_Yout;
uint16_t Accel_Zout;

uint8_t Accel_Xout_H;
uint8_t Accel_Xout_L;
uint8_t Accel_Yout_H;
uint8_t Accel_Yout_L;
uint8_t Accel_Zout_H;
uint8_t Accel_Zout_L;

int i = 0;
int j = 0;
char buff[100];
int len;

int rxIndx;
char rxData[2];
char rxBuff[20];
char transferCplt;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);

#ifdef __GNUC__
	#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
	#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif

PUTCHAR_PROTOTYPE
{
	HAL_UART_Transmit(&huart1, (uint8_t*)&ch, 1, 100);
	return ch;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	uint8_t i;
	if(huart -> Instance == USART1) //aktualny UART
	{
		if(rxIndx == 0)
		{
			for(i = 0; i < 20; i++) rxBuff[i] = 0; // czyszczenie buforu rx
		}
		if(rxData[0] != 64)
		{
			rxBuff[rxIndx++] = rxData[0]; // dodawanie danych do bufora
		}
		else
		{
			HAL_GPIO_TogglePin(GPIOC, LD3_Pin);
			rxIndx = 0;
			transferCplt = 1; // transfer skończony, dane gotowe do odczytu
		}
		HAL_UART_Receive_IT(&huart1, rxData, 1); // aktywowanie przerwania uart dla każdego 1 bajta
	}
}

void send_string(char* s)
{
  HAL_UART_Transmit(&huart1, (uint8_t*)s, strlen(s), 1000);
}

uint8_t ReadRegister(uint8_t regAddres)
{
	uint8_t value = 0;
	HAL_I2C_Mem_Read(&hi2c1, MPU9250_Address << 1, regAddres, 1, &value, sizeof(value), 100);
	return value;
}

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_I2C1_Init();

  MX_USART1_UART_Init();
  HAL_UART_Receive_IT(&huart1, rxData, 1);

  uint8_t i = 0;

  buffer = 0xa0;
  HAL_I2C_Mem_Write(&hi2c1, MPU9250_Address << 1, PWR_MGMT_1_Address, 1, &buffer, 1, 100);

  buffer = 0xe0;
  HAL_I2C_Mem_Write(&hi2c1, MPU9250_Address << 1, ACCEL_CONFIG_Address, 1, &buffer, 1, 100);
  buffer = 0x00;
  HAL_I2C_Mem_Write(&hi2c1, MPU9250_Address << 1, ACCEL_CONFIG2_Address, 1, &buffer, 1, 100);
  buffer = 0x0b;
  HAL_I2C_Mem_Write(&hi2c1, MPU9250_Address << 1, LP_ACCEL_ODR_Address, 1, &buffer, 1, 100);

  while (1)
  {
	  readReg = ReadRegister(WHO_AM_I_Address);
	  if(readReg != 0)
	  {
		  HAL_GPIO_WritePin(GPIOC, LD3_Pin, GPIO_PIN_SET);
		  isRegWorking = readReg;
	  }
	  else
	  {
		  HAL_GPIO_WritePin(GPIOC, LD3_Pin, GPIO_PIN_RESET);
	  }
	  HAL_Delay(10);
	  j++;
	  if(isRegWorking != 0)
	  {
		  rawTemp = (ReadRegister(TEMP_OUT_H_Address) << 8) + ReadRegister(TEMP_OUT_L_Address);
		  TEMP_degC += (rawTemp * 125.0) / INT16_MAX;
		  i++;
		  if (i == 100)
		  {
			  FinalTEMP_degC = (TEMP_degC / 100) - (9.0 / 1.0) + 21;
			  TEMP_degC = 0;
			  i = 0;
		  }
		  Accel_Xout = (ReadRegister(ACCEL_XOUT_H_Address) << 8) + ReadRegister(ACCEL_XOUT_L_Address);
		  Accel_Yout = (ReadRegister(ACCEL_YOUT_H_Address) << 8) + ReadRegister(ACCEL_YOUT_L_Address);
		  Accel_Zout = (ReadRegister(ACCEL_ZOUT_H_Address) << 8) + ReadRegister(ACCEL_ZOUT_L_Address);

		  Accel_X_Result = ((float)Accel_Xout * ACC_RESOLUTION) / INT16_MAX;
		  Accel_Y_Result = ((float)Accel_Yout * ACC_RESOLUTION) / INT16_MAX;
		  Accel_Z_Result = ((float)Accel_Zout * ACC_RESOLUTION) / INT16_MAX;

		  if(j >= 20)
		  {
		  		  HAL_GPIO_TogglePin(GPIOC, LD4_Pin);

		  		  AR_Sign = (Accel_X_Result < 0) ? "-" : "";
		  		  AR_Int = Accel_X_Result;
		  		  AR_Float = Accel_X_Result - AR_Int;
		  		  AR_Int2 = trunc(AR_Float * 10000);
		  		  sprintf(buff, "W osi x: %s%d.%04d \r\n", AR_Sign, AR_Int, AR_Int2);
		  		  len = strlen(buff);
		  		  HAL_UART_Transmit(&huart1, buff, len, 1000);

		  		  AR_Sign = (Accel_Y_Result < 0) ? "-" : "";
		  		  AR_Int = Accel_Y_Result;
		  		  AR_Float = Accel_Y_Result - AR_Int;
		  		  AR_Int2 = trunc(AR_Float * 10000);
		  		  sprintf(buff, "W osi y: %s%d.%04d \r\n", AR_Sign, AR_Int, AR_Int2);
		   		  len = strlen(buff);
		   		  HAL_UART_Transmit(&huart1, buff, len, 1000);

		  		  AR_Sign = (Accel_Z_Result < 0) ? "-" : "";
		  		  AR_Int = Accel_Z_Result;
		  		  AR_Float = Accel_Z_Result - AR_Int;
		  		  AR_Int2 = trunc(AR_Float * 10000);
		  		  sprintf(buff, "W osi z: %s%d.%04d \r\n", AR_Sign, AR_Int, AR_Int2);
		  		  len = strlen(buff);
		  		  HAL_UART_Transmit(&huart1, buff, len, 1000);

		  		  transferCplt = 0;
		  		  j = 0;
		  }
	  }
  }
}

void MPU9250_Init(uint8_t u8asax[3])
{

}

void MPU9250_GetData()
{

}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00003D84;
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

  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }

}

static void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, LD4_Pin|LD3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD4_Pin LD3_Pin */
  GPIO_InitStruct.Pin = LD4_Pin|LD3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

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

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(char *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
