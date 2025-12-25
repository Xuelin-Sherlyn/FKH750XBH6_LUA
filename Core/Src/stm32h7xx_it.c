/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32h7xx_it.c
  * @brief   Interrupt Service Routines.
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
#include "stm32h7xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usart.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

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
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern MDMA_HandleTypeDef hmdma_quadspi_fifo_th;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;
extern UART_HandleTypeDef huart1;
extern TIM_HandleTypeDef htim17;

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
   while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */
  safe_printf("\r\n\033[31mHardFault Call!!!\033[0m\n");
  // 获取所有相关寄存器
  uint32_t cfsr = SCB->CFSR;
  uint32_t hfsr = SCB->HFSR; 
  uint32_t mmfar = SCB->MMFAR;
  uint32_t bfar = SCB->BFAR;
  uint32_t shcsr = SCB->SHCSR;
  
  safe_printf("\rCFSR:  0x%08lX\n", (unsigned long)cfsr);
  safe_printf("\rHFSR:  0x%08lX\n", (unsigned long)hfsr);
  safe_printf("\rMMFAR: 0x%08lX\n", (unsigned long)mmfar);
  safe_printf("\rBFAR:  0x%08lX\n", (unsigned long)bfar);
  safe_printf("\rSHCSR: 0x%08lX\n", (unsigned long)shcsr);
  
  // 详细解析 CFSR
  if(cfsr & (1 << 0))  safe_printf("\r  IACCVIOL: Instruction access violation\n");
  if(cfsr & (1 << 1))  safe_printf("\r  DACCVIOL: Data access violation\n");
  if(cfsr & (1 << 3))  safe_printf("\r  MUNSTKERR: MemManage on unstacking\n");
  if(cfsr & (1 << 4))  safe_printf("\r  MSTKERR: MemManage on stacking\n");
  if(cfsr & (1 << 5))  safe_printf("\r  MLSPERR: MemManage FPU lazy state\n");
  if(cfsr & (1 << 7))  safe_printf("\r  MMARVALID: MMFAR is valid\n");
  if(cfsr & (1 << 8))  safe_printf("\r  IBUSERR: Instruction bus error\n");
  if(cfsr & (1 << 9))  safe_printf("\r  PRECISERR: Precise data bus error\n");
  if(cfsr & (1 << 10)) safe_printf("\r  IMPRECISERR: Imprecise data bus error\n");
  if(cfsr & (1 << 11)) safe_printf("\r  UNSTKERR: Bus fault on unstacking\n");
  if(cfsr & (1 << 12)) safe_printf("\r  STKERR: Bus fault on stacking\n");
  if(cfsr & (1 << 13)) safe_printf("\r  LSPERR: Bus fault FPU lazy state\n");
  if(cfsr & (1 << 15)) safe_printf("\r  BFARVALID: BFAR is valid\n");
  if(cfsr & (1 << 16)) safe_printf("\r  UNDEFINSTR: Undefined instruction\n");
  if(cfsr & (1 << 17)) safe_printf("\r  INVSTATE: Invalid state\n");
  if(cfsr & (1 << 18)) safe_printf("\r  INVPC: Invalid PC load\n");
  if(cfsr & (1 << 19)) safe_printf("\r  NOCP: No coprocessor\n");
  if(cfsr & (1 << 24)) safe_printf("\r  DIVBYZERO: Division by zero\n");
  if(cfsr & (1 << 25)) safe_printf("\r  UNALIGNED: Unaligned access\n");
  
  // 获取调用栈信息
  uint32_t *msp = (uint32_t *)__get_MSP();
  uint32_t *psp = (uint32_t *)__get_PSP();
  
  safe_printf("\rMSP: 0x%08lX\n", (unsigned long)(uint32_t)msp);
  safe_printf("\rPSP: 0x%08lX\n", (unsigned long)(uint32_t)psp);
  
  // 打印栈内容（可能包含返回地址）
  safe_printf("\rStack dump (MSP):\n");
  for(int i = 0; i < 16; i++) {
      safe_printf("\r  [0x%08lX]: 0x%08lX\n", (unsigned long)(uint32_t)(msp + i), (unsigned long)msp[i]);
  }
  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/******************************************************************************/
/* STM32H7xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32h7xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles DMA1 stream0 global interrupt.
  */
void DMA1_Stream0_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Stream0_IRQn 0 */

  /* USER CODE END DMA1_Stream0_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_usart1_rx);
  /* USER CODE BEGIN DMA1_Stream0_IRQn 1 */

  /* USER CODE END DMA1_Stream0_IRQn 1 */
}

/**
  * @brief This function handles DMA1 stream1 global interrupt.
  */
void DMA1_Stream1_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Stream1_IRQn 0 */

  /* USER CODE END DMA1_Stream1_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_usart1_tx);
  /* USER CODE BEGIN DMA1_Stream1_IRQn 1 */

  /* USER CODE END DMA1_Stream1_IRQn 1 */
}

/**
  * @brief This function handles USART1 global interrupt.
  */
void USART1_IRQHandler(void)
{
  /* USER CODE BEGIN USART1_IRQn 0 */

  /* USER CODE END USART1_IRQn 0 */
  HAL_UART_IRQHandler(&huart1);
  /* USER CODE BEGIN USART1_IRQn 1 */

  /* USER CODE END USART1_IRQn 1 */
}

/**
  * @brief This function handles TIM17 global interrupt.
  */
void TIM17_IRQHandler(void)
{
  /* USER CODE BEGIN TIM17_IRQn 0 */

  /* USER CODE END TIM17_IRQn 0 */
  HAL_TIM_IRQHandler(&htim17);
  /* USER CODE BEGIN TIM17_IRQn 1 */

  /* USER CODE END TIM17_IRQn 1 */
}

/**
  * @brief This function handles MDMA global interrupt.
  */
void MDMA_IRQHandler(void)
{
  /* USER CODE BEGIN MDMA_IRQn 0 */

  /* USER CODE END MDMA_IRQn 0 */
  HAL_MDMA_IRQHandler(&hmdma_quadspi_fifo_th);
  /* USER CODE BEGIN MDMA_IRQn 1 */

  /* USER CODE END MDMA_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
