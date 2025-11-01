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
#include "adc.h"
#include "cdc_device.h"
#include "fdcan.h"
#include "gpio.h"
#include "i2c.h"
#include "spi.h"
#include "sst.h"
#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_gpio.h"
#include "usb_otg.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "analog_sensors.h"
#include "can.h"
#include "can_defs.h"
#include "fifo.h"
#include "inttypes.h"
#include "scheduler.h"
#include "stm32h7xx_hal_i2c.h"
#include "tusb.h"
#include "usb_descriptors.c"
#include "usb_task.h"
#include "usbd.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

void set_leds(int red, int green, int blue) {
  if (blue >= 0)
    HAL_GPIO_WritePin(blue_led_GPIO_Port, blue_led_Pin, !blue);
  if (green >= 0)
    HAL_GPIO_WritePin(green_led_GPIO_Port, green_led_Pin, !green);
  if (red >= 0)
    HAL_GPIO_WritePin(red_led_GPIO_Port, red_led_Pin, !red);
}
int __io_putchar(int ch) {
  if (tud_cdc_connected())
    fifo_push(&usb_fifo, ch); // defined in usb_taks.h
  return ch;
}

void usb_fifo_error_callback() {
  set_leds(true, -1, -1); //
}

/* USER CODE END PTD */

/* Private define
 * ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro
 * -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables
 * ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes
 * -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */

uint8_t raw_pix_buffer[128] = {0};
uint16_t pixel_buffer[64] = {0};

/* USER CODE END PFP */

/* Private user code
 * ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void Enable_USB_IRQs() {
  HAL_NVIC_SetPriority(OTG_FS_EP1_OUT_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(OTG_FS_EP1_OUT_IRQn);
  HAL_NVIC_SetPriority(OTG_FS_EP1_IN_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(OTG_FS_EP1_IN_IRQn);
  HAL_NVIC_SetPriority(OTG_FS_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(OTG_FS_IRQn);
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c) {
  for (int i = 0; i < 64; i++) {
    uint8_t pos = i << 1;
    uint16_t recast = ((uint16_t)raw_pix_buffer[pos + 1] << 8) |
                      ((uint16_t)raw_pix_buffer[pos]);

    pixel_buffer[i] = recast / 4; // TODO: div by 4 is the conversion factor,
                                  // but it truncates decimals and isn't signed
  }
  // printf("pixels: %d, %d, %d, %d\r\n", pixel_buffer[0], pixel_buffer[1],
  //        pixel_buffer[2], pixel_buffer[3]);
}

// ***********************************************************************
// The HAL PCD IRQ Handler MUST NOT be called, or TinyUSB won't work
// Regenerating sources WILL put it back in, so we don't have cubemx generate
// this or enable interrupts
// ***********************************************************************
void OTG_FS_IRQHandler(void) {
  tud_int_handler(0);
  // HAL_GPIO_TogglePin(blue_led_GPIO_Port, blue_led_Pin);
}
void OTG_FS_EP1_OUT_IRQHandler(void) {};
void OTG_FS_EP1_IN_IRQHandler(void) {};

void scan_i2c_bus() {
  uint8_t possible_dev_addr[] = {
      0x68,
      0x69,
  };
  const uint8_t timeout = 10;
  for (int i = 0; i < 2; i++) {
    uint8_t dev_addr = possible_dev_addr[i];
    HAL_StatusTypeDef status =
        HAL_I2C_Master_Transmit(&hi2c1, dev_addr << 1, NULL, 0, timeout);

    if (status == HAL_OK) {
      // Device is present and ACKed.
      printf("i2c device present at: 0x%x\r\n", dev_addr);
    } else {
      // Device is not present or NACKed.
      printf("i2c device NOT present at: 0x%x\r\n", dev_addr);
    }
  }
}

INIT_AnalogMsg(can_msg);

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU
   * Configuration--------------------------------------------------------*/
  MPU_Config();

  /* Enable the CPU Cache */

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();

  /* MCU
   * Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the
   * Systick.
   */
  HAL_Init();

  /* USER CODE BEGIN Init */
  can_msg.data.analog.throttle_position = 1;
  can_msg.data.analog.rear_brake_pressure = 0;
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_FDCAN1_Init();
  MX_I2C2_Init();
  MX_SPI2_Init();
  MX_USB_OTG_FS_PCD_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  Enable_USB_IRQs();
  tusb_rhport_init_t dev_init = {.role = TUSB_ROLE_DEVICE,
                                 .speed = TUSB_SPEED_AUTO};
  tusb_init(0, &dev_init);
  set_leds(false, false, false);

  config_can();
  usb_task_instantiate(usb_fifo_error_callback);
  static SST_Evt const *usbQSto[10];   /* Event queue storage */
  SST_Task_start(AO_usb,               /* AO pointer to start */
                 1U,                   /* SST-priority */
                 usbQSto,              /* storage for the AO's queue */
                 ARRAY_NELEM(usbQSto), /* queue length */
                 (void *)0);           /* initialization event (not used) */
  analog_task_instantiate(
      usb_fifo_error_callback); // TODO: error callback currently unused

  static SST_Evt const *analogQSto[10];   /* Event queue storage */
  SST_Task_start(AO_analog,               /* AO pointer to start */
                 1U,                      /* SST-priority */
                 analogQSto,              /* storage for the AO's queue */
                 ARRAY_NELEM(analogQSto), /* queue length */
                 (void *)0);              /* initialization event (not used) */
  SST_start();
  SST_onStart(); /* application callback to config & start interrupts */
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    printf("\r\n");
    for (uint16_t i = 0; i < 768; i++) {
      printf("%d ", i);
    }
    printf("\r\n");

    // uint8_t buffer[1] = {0x80};
    // HAL_I2C_Master_Transmit(&hi2c1, 0x69 << 1, buffer, 1, 1000);
    // HAL_I2C_Master_Receive_IT(&hi2c1, 0x69 << 1, raw_pix_buffer, 128);

    HAL_GPIO_TogglePin(green_led_GPIO_Port, green_led_Pin);
    // board_delay(10);
    HAL_Delay(10);
    HAL_GPIO_TogglePin(green_led_GPIO_Port, green_led_Pin);
    // board_delay(990);
    HAL_Delay(990);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
   */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
   */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {
  }

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 2;
  RCC_OscInitStruct.PLL.PLLN = 240;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 20;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 |
                                RCC_CLOCKTYPE_D3PCLK1 | RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
    Error_Handler();
  }

  /** Enables the Clock Security System
   */
  HAL_RCC_EnableCSS();
}

/**
 * @brief Peripherals Common Clock Configuration
 * @retval None
 */
void PeriphCommonClock_Config(void) {
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
   */
  PeriphClkInitStruct.PeriphClockSelection =
      RCC_PERIPHCLK_ADC | RCC_PERIPHCLK_SPI2;
  PeriphClkInitStruct.PLL2.PLL2M = 4;
  PeriphClkInitStruct.PLL2.PLL2N = 160;
  PeriphClkInitStruct.PLL2.PLL2P = 8;
  PeriphClkInitStruct.PLL2.PLL2Q = 2;
  PeriphClkInitStruct.PLL2.PLL2R = 2;
  PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_1;
  PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
  PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
  PeriphClkInitStruct.Spi123ClockSelection = RCC_SPI123CLKSOURCE_PLL2;
  PeriphClkInitStruct.AdcClockSelection = RCC_ADCCLKSOURCE_PLL2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* MPU Configuration */

void MPU_Config(void) {
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
   */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state
   */
  __disable_irq();
  HAL_GPIO_WritePin(blue_led_GPIO_Port, blue_led_Pin, true);
  HAL_GPIO_WritePin(green_led_GPIO_Port, green_led_Pin, true);
  HAL_GPIO_WritePin(red_led_GPIO_Port, red_led_Pin, true);
  while (1) {
    HAL_GPIO_TogglePin(blue_led_GPIO_Port, blue_led_Pin);
    HAL_GPIO_TogglePin(green_led_GPIO_Port, green_led_Pin);
    HAL_GPIO_TogglePin(red_led_GPIO_Port, red_led_Pin);
    HAL_Delay(100);
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
void assert_failed(uint8_t *file, uint32_t line) {
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number,
     ie: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
