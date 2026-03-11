#include "main.h"

static void SystemClock_Config(void);

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  while (1)
  {
  }
}

static void SystemClock_Config(void)
{
  RCC_OscInitTypeDef rcc_osc_init = {0};
  RCC_ClkInitTypeDef rcc_clk_init = {0};

  rcc_osc_init.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  rcc_osc_init.HSIState = RCC_HSI_ON;
  rcc_osc_init.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  rcc_osc_init.PLL.PLLState = RCC_PLL_OFF;

  if (HAL_RCC_OscConfig(&rcc_osc_init) != HAL_OK)
  {
    Error_Handler();
  }

  rcc_clk_init.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                           RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  rcc_clk_init.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  rcc_clk_init.AHBCLKDivider = RCC_SYSCLK_DIV1;
  rcc_clk_init.APB1CLKDivider = RCC_HCLK_DIV1;
  rcc_clk_init.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&rcc_clk_init, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

void Error_Handler(void)
{
  __disable_irq();

  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  (void)file;
  (void)line;

  Error_Handler();
}
#endif
