
#include "main.h"
#include "stm32wlxx_hal.h"

#include "stm32wlxx_hal_rcc.h"
#include "stm32wlxx_hal_rcc_ex.h"
#include "stm32wlxx_hal_pwr_ex.h"

#include "stm32wlxx_hal_tim.h"
#include "stm32wlxx_hal_tim_ex.h"
#include "app_lorawan.h"
#include "sys_app.h"
//#include "i2c.h"
#include "sys_sensors.h"
#include "stm32wlxx_hal_spi.h"
#include "st7789.h"
#include "string.h"
#include "lora_app.h"
#include "sys_app.h"
#include "stm32_seq.h"

void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_SPI1_Init(void);

SPI_HandleTypeDef hspi1;
TIM_HandleTypeDef htim2;


#define DS18B20_PORT GPIOA
#define DS18B20_PIN GPIO_PIN_8
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

extern UART_HandleTypeDef huart2;


/* USER CODE BEGIN PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
//void SystemClock_Config(void);
//static void MX_GPIO_Init(void);
//static void MX_TIM2_Init(void);
//static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

void delay_us(uint32_t us)
{
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while (__HAL_TIM_GET_COUNTER(&htim2) < us);
}





//void delay_us(uint16_t us)
//{
//    uint32_t start = DWT->CYCCNT;
//    uint32_t ticks = us * (HAL_RCC_GetHCLKFreq() / 1000000);
//    while ((DWT->CYCCNT - start) < ticks);
//}

uint8_t DS18B20_Start(void)
{
    uint8_t response = 0;
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Set pin as output
    GPIO_InitStruct.Pin = DS18B20_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DS18B20_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
    delay_us(480);

    // Set pin as input
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(DS18B20_PORT, &GPIO_InitStruct);

    delay_us(80);
    if (!HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN)) response = 1;
    else response = 0;
    delay_us(400);
    return response;
}

void DS18B20_Write(uint8_t data)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    for (int i = 0; i < 8; i++)
    {
        // Set as output
        GPIO_InitStruct.Pin = DS18B20_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        HAL_GPIO_Init(DS18B20_PORT, &GPIO_InitStruct);

        HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
        delay_us(1);

        if (data & (1 << i))
        {
            HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
            delay_us(60);
        }
        else
        {
            delay_us(60);
            HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
        }
    }
}

uint8_t DS18B20_Read(void)
{
    uint8_t value = 0;
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    for (int i = 0; i < 8; i++)
    {
        // Set as output
        GPIO_InitStruct.Pin = DS18B20_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        HAL_GPIO_Init(DS18B20_PORT, &GPIO_InitStruct);

        HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
        delay_us(2);

        // Set as input
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        HAL_GPIO_Init(DS18B20_PORT, &GPIO_InitStruct);

        if (HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN))
            value |= (1 << i);

        delay_us(60);
    }
    return value;
}

float DS18B20_GetTemp(void)
{
    uint8_t temp_l, temp_h;
    int16_t temp;
    float temperature = 0;

    DS18B20_Start();
    DS18B20_Write(0xCC);  // Skip ROM
    DS18B20_Write(0x44);  // Convert T
    HAL_Delay(750);

    DS18B20_Start();
    DS18B20_Write(0xCC);  // Skip ROM
    DS18B20_Write(0xBE);  // Read Scratchpad

    temp_l = DS18B20_Read();
    temp_h = DS18B20_Read();

    temp = (temp_h << 8) | temp_l;
    temperature = (float)temp / 16.0;

    return temperature;
}


int main(void)
{
	HAL_Init();
	SystemClock_Config();
//	MX_I2C2_Init();
	SystemApp_Init();

	/* Initialize the Sensors *******************/
	EnvSensors_Init();
	MX_TIM2_Init();
	/* LCD init *********************************/
//	LCD_Buffer_Init();
//	MX_GPIO_Init();
//	MX_SPI1_Init();
//	ST7789_Init();

	/* LCD test *********************************/
//	lcd_printf(LCD_BLUE, "Device turned On");
//	lcd_printf(LCD_BLUE, "Init LoRaWAN Stack...");
//	lcd_print_buf();
	  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

	  char msg[50];
	  sprintf(msg, "Temperature: °C\r\n");
	  HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
	LoRaWAN_Init();

	while (1)
	{
	      float temp = DS18B20_GetTemp();
	      sprintf(msg, "Temperature: %.2f °C\r\n", temp);
	      HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
	      HAL_Delay(1000);
		//MX_LoRaWAN_Process();
	}
}












void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

 // __HAL_RCC_PWR_CLK_ENABLE();
  HAL_PWR_EnableBkUpAccess();

  // --- Turn ON HSI and LSE ---
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;

  // --- PLL config for 48 MHz SYSCLK ---
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 6;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    Error_Handler();

  // --- Bus clocks ---
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK3 |
                                RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLK3Divider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    Error_Handler();

  // --- Select LSE as RTC clock source ---
  __HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_LSE);
  __HAL_RCC_RTC_ENABLE();
}


 void MX_TIM2_Init(void)
{
  __HAL_RCC_TIM2_CLK_ENABLE();  // Enable clock for TIM2
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 47;                   // 48 MHz / (47+1) = 1 MHz → 1 tick = 1 µs
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 0xFFFFFFFF;              // Max 32-bit counter
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_TIM_Base_Start(&htim2);   // Start the timer
}








//
//void SystemClock_Config(void)
//{
//	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
//	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
//
//	/** Configure LSE Drive Capability
//	 */
//	HAL_PWR_EnableBkUpAccess();
//	__HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);
//	/** Configure the main internal regulator output voltage
//	 */
//	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
//	/** Initializes the CPU, AHB and APB busses clocks
//	 */
//	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE|RCC_OSCILLATORTYPE_MSI;
//	RCC_OscInitStruct.LSEState = RCC_LSE_ON;
//	RCC_OscInitStruct.MSIState = RCC_MSI_ON;
//	RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
//	RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_11;
//	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
//	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
//	{
//		Error_Handler();
//	}
//	/** Configure the SYSCLKSource, HCLK, PCLK1 and PCLK2 clocks dividers
//	 */
//	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK3|RCC_CLOCKTYPE_HCLK
//			|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1
//			|RCC_CLOCKTYPE_PCLK2;
//	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
//	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
//	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
//	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
//	RCC_ClkInitStruct.AHBCLK3Divider = RCC_SYSCLK_DIV1;
//
//	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
//	{
//		Error_Handler();
//	}
//}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
void MX_SPI1_Init(void)
{
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
void MX_GPIO_Init(void)
{
  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

	GPIO_InitTypeDef CS_init;
	CS_init.Mode = GPIO_MODE_OUTPUT_PP;
	CS_init.Pin = GPIO_PIN_2;
	CS_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(GPIOC, &CS_init);

	GPIO_InitTypeDef RST_init;
	RST_init.Mode = GPIO_MODE_OUTPUT_PP;
	RST_init.Pin = GPIO_PIN_2;
	RST_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(GPIOB, &RST_init);

	GPIO_InitTypeDef DC_init;
	DC_init.Mode = GPIO_MODE_OUTPUT_PP;
	DC_init.Pin = GPIO_PIN_10;
	DC_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(GPIOB, &DC_init);
}

void Error_Handler(void)
{
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1)
	{
	}
}
