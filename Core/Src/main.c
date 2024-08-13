/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  *
  * Sebkabob's WatchDog Code
  * 2024
  *
  * @brief          : Main program body
  ******************************************************************************
  *
  * The WatchDog is a small security tag that detect movements with its built in
  * Inertia Measurement Unit, when a movement is detected, the WatchDog will sound
  * an alarm to alert everyone nearby that the tag is being tampered with
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "gpio.h"
// made a comment
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lis3dh_reg.h"
#include <string.h>
#include <stdio.h>

#define SENSOR_BUS hi2c1

uint8_t *bufp1;

#define LIS3DH_ADDRESS 0x18

static uint8_t whoamI;

int debounce = 250;

static int16_t data_raw_acceleration[3];
static int16_t data_raw_temperature;
static float acceleration_mg[3];
static float temperature_degC;

int theCode(int N1, int N2, int N3, int N4) {
    // Check if the light state is equal to the code input to the function
    if ((HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_11) != N1) && (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6) != N2)
    	&& (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7) != N3) && (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6) != N4)) {
        return 1;
    } else {
        return 0;
    }
}

void allState(int state) {
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, state);
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, state);
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, state);
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, state);
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, state);
	  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, state);
	  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, state);
}

void BeepX(float seconds, float beepNumPerSecond) {
	float j = seconds * beepNumPerSecond;
	float t = 1000.0 / beepNumPerSecond;
	float x = t/2.0;
	for (float i = 0; i <= j; i++){
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, 1);
		HAL_Delay(x);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, 0);
		HAL_Delay(x);
	}
}

void buttonsNlights() {
	if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8) == 0)
	{
		HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_11);
		HAL_Delay(debounce);
	}
	if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4) == 0)
	{
		HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_6);
		HAL_Delay(debounce);
	}
	if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == 0)
	{
		HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_7);
		HAL_Delay(debounce);
	}
	if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == 0)
	{
		HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
		HAL_Delay(debounce);
	}
}

void ErrorSound() {
	BeepX(0.5,698);
	BeepX(0.5,520);
	BeepX(0.5,350);
	HAL_Delay(1000);
}

void GoodSound() {
	BeepX(0.5,520);
	BeepX(0.5,660);
	BeepX(0.5,780);
	HAL_Delay(500);
}


int8_t twiScan() {
	for (uint8_t i = 0; i < 128; i++) {
		if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(i<<1), 3, 5) == HAL_OK) {
			  // We got an ack
			if (i == LIS3DH_ADDRESS) {
				return 1;
			}
		}
	}
	return 0;
}

//static uint8_t tx_buffer[1000];

/** Please note that is MANDATORY: return 0 -> no Error.**/
int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len) {
	reg |= 0x80;
	HAL_I2C_Mem_Write(handle, 0x30, reg, 1, (uint8_t*) bufp, len, 1000); //define 8bit
}

int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len) {
	reg |= 0x80;
	HAL_I2C_Mem_Read(handle, 0x30, reg, 1, bufp, len, 1000);
}

/** Optional (may be required by driver) **/
void platform_delay(uint32_t millisec) {
	HAL_Delay(millisec);
}


void UpdateValues(void)
{
  stmdev_ctx_t dev_ctx;
  dev_ctx.write_reg = platform_write;
  dev_ctx.read_reg = platform_read;
  dev_ctx.mdelay = platform_delay;
  dev_ctx.handle = &SENSOR_BUS;
  platform_delay(10);
  lis3dh_device_id_get(&dev_ctx, &whoamI);

  if (whoamI != LIS3DH_ID) {
    while (1) {
    	ErrorSound();
    }
  }

  /* Enable Block Data Update. */
  lis3dh_block_data_update_set(&dev_ctx, PROPERTY_ENABLE);
  /* Set Output Data Rate to 1Hz. */
  lis3dh_data_rate_set(&dev_ctx, LIS3DH_ODR_10Hz);
  /* Set full scale to 2g. */
  lis3dh_full_scale_set(&dev_ctx, LIS3DH_2g);
  /* Enable temperature sensor. */
  lis3dh_aux_adc_set(&dev_ctx, LIS3DH_AUX_ON_TEMPERATURE);
  /* Set device in continuous mode with 12 bit resol. */
  lis3dh_operating_mode_set(&dev_ctx, LIS3DH_HR_12bit);

    lis3dh_reg_t reg;
    /* Read output only if new value available */
    lis3dh_xl_data_ready_get(&dev_ctx, &reg.byte);

    if (reg.byte) {
      memset(data_raw_acceleration, 0x00, 3 * sizeof(int16_t));
      lis3dh_acceleration_raw_get(&dev_ctx, data_raw_acceleration);
      acceleration_mg[0] =
        lis3dh_from_fs2_hr_to_mg(data_raw_acceleration[0]);
      acceleration_mg[1] =
        lis3dh_from_fs2_hr_to_mg(data_raw_acceleration[1]);
      acceleration_mg[2] =
        lis3dh_from_fs2_hr_to_mg(data_raw_acceleration[2]);
    }

    lis3dh_temp_data_ready_get(&dev_ctx, &reg.byte);

    if (reg.byte) {
      /* Read temperature data */
      memset(&data_raw_temperature, 0x00, sizeof(int16_t));
      lis3dh_temperature_raw_get(&dev_ctx, &data_raw_temperature);
      temperature_degC = lis3dh_from_lsb_hr_to_celsius(data_raw_temperature);
    }
}

void Bar4() {
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, 0);
	  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, 0);
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, 0);
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, 0);
}
void Bar3() {
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, 1);
	  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, 0);
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, 0);
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, 0);
}
void Bar2() {
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, 1);
	  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, 1);
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, 0);
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, 0);
}
void Bar1() {
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, 1);
	  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, 1);
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, 1);
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, 0);
}


void TempLightBar() {
	  UpdateValues();
	  if (temperature_degC > 32) {
		  Bar4();
	  } else if (temperature_degC > 30.5) {
		  Bar3();
	  } else if (temperature_degC > 29) {
		  Bar2();
	  } else if (temperature_degC > 27.5) {
		  Bar1();
	  } else if (temperature_degC < 27.5) {
		  allState(1);
	  }
}


void AccelDetecting() {
	  Bar4();
	  HAL_Delay(750);
	  Bar3();
	  HAL_Delay(750);
	  Bar2();
	  HAL_Delay(750);
	  Bar1();
	  HAL_Delay(750);
	  allState(1);
	  UpdateValues();
	  int Xinit = acceleration_mg[0];
	  int Yinit = acceleration_mg[1];
	  int Zinit = acceleration_mg[2];
	  int j = 1;
	  while (j == 1) {
		  HAL_Delay(20);
		  UpdateValues();
		  HAL_Delay(20);
		  int Xcurr = acceleration_mg[0];
		  int Ycurr = acceleration_mg[1];
		  int Zcurr = acceleration_mg[2];
		  int Trig5 = 300;
		  int Trig4 = 250;
		  int Trig3 = 200;
		  int Trig2 = 100;
		  int Trig1 = 50;

		  if (Xcurr - Xinit > Trig5 || Ycurr - Yinit > Trig5 || Zcurr - Zinit > Trig5) {
			  //BeepX(1,40);
			  allState(0);
			  HAL_Delay(300);
			  //HAL_Delay(500);
			  //j = 2;
		  }
		  else if (Xcurr - Xinit > Trig4 || Ycurr - Yinit > Trig4 || Zcurr - Zinit > Trig4) {
			  Bar4();
		  }
		  else if (Xcurr - Xinit > Trig3 || Ycurr - Yinit > Trig3 || Zcurr - Zinit > Trig3) {
			  Bar3();
		  }
		  else if (Xcurr - Xinit > Trig2 || Ycurr - Yinit > Trig2 || Zcurr - Zinit > Trig2) {
			  Bar2();
		  }
		  else if (Xcurr - Xinit > Trig1 || Ycurr - Yinit > Trig1 || Zcurr - Zinit > Trig1) {
			  Bar1();
		  }
		  else if (Xcurr - Xinit < Trig1 || Ycurr - Yinit < Trig1 || Zcurr - Zinit < Trig1) {
			  allState(1);
		  }

	  }
}


///////////////////////////////////////////////////////////////////

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

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

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
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  allState(1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  buttonsNlights();
	  if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == 0 && theCode(1,0,1,0)) {
		  AccelDetecting();
	  }

//	  buttonsNlights();
//	  if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == 0) // Assuming active low
//	  {
//		  if (theCode(1,0,1,0)) //button 1, button 2, button 3, button 4
//		  {
//			  int time = 1; // in seconds
//			  int q = time * 5;
//			  for (int i = 0 ; i <= q; i++)
//			  {
//				  HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_1);
//				  HAL_Delay(200);
//			  }
//			  allState(1);
//		  }
//			  else
//			  {
//				  allState(1);
//				  UpdateValues();
//				  if (temperature_degC > 33) {
//					  HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_11);
//					  HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_6);
//					  HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_7);
//					  HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
//					  HAL_Delay(500);
//				  } else if (temperature_degC > 31) {
//					  HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_6);
//					  HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_7);
//					  HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
//					  HAL_Delay(500);
//				  } else if (temperature_degC > 29) {
//					  HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_7);
//					  HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
//					  HAL_Delay(500);
//				  } else if (temperature_degC < 27) {
//					  HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
//					  HAL_Delay(500);
//				  }
//				  allState(1);
//
//			  }
//	  }
  }

//enter code
	  //code directs to mode    -    if not real code, go back to enter code
	  //mode waits for interrupt
	  //when interrupt -> wait 3 seconds -> alarm state
	  //gives 3 seconds before alarm state to enterCode();
	  //

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
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

#ifdef  USE_FULL_ASSERT
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
