/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usart.h"
#include "uart_dyn_rx.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define AXIRAM_ADDR        ((uint8_t*)0x24000000)
#define AXIRAM_SIZE        ((uint32_t)0x00080000)
#define SDRAM_ADDR         ((uint8_t*)0xC0000000)
#define SDRAM_SIZE         ((uint32_t)0x01000000)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
static HeapRegion_t HeapRAMRegions[]=
{
  {AXIRAM_ADDR, AXIRAM_SIZE},
  {SDRAM_ADDR, SDRAM_SIZE},
};
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for LUA_ProcessTask */
osThreadId_t LUA_ProcessTaskHandle;
const osThreadAttr_t LUA_ProcessTask_attributes = {
  .name = "LUA_ProcessTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void LUA_ProcessTask_Handle(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void configureTimerForRunTimeStats(void);
unsigned long getRunTimeCounterValue(void);

/* USER CODE BEGIN 1 */
/* Functions needed when configGENERATE_RUN_TIME_STATS is on */
void configureTimerForRunTimeStats(void)
{
	/* 启用 DWT 功能 */
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  
  /* 重置并启用周期计数器 */
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

unsigned long getRunTimeCounterValue(void)
{
	/* 直接返回当前周期计数值 */
  return DWT->CYCCNT;
}
/* USER CODE END 1 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  vPortDefineHeapRegions(HeapRAMRegions);
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of LUA_ProcessTask */
  LUA_ProcessTaskHandle = osThreadNew(LUA_ProcessTask_Handle, NULL, &LUA_ProcessTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_LUA_ProcessTask_Handle */
/**
* @brief Function implementing the LUA_ProcessTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_LUA_ProcessTask_Handle */
void LUA_ProcessTask_Handle(void *argument)
{
  /* USER CODE BEGIN LUA_ProcessTask_Handle */
  /* Infinite loop */
  UART_DataPacket_t packet;
  UART_Dynamic_Receive_Init();
  safe_printf("\033[36mLua Shell> \033[0m");
  /* Infinite loop */
  for(;;)
  {
     if (xQueueReceive(UART_Receiver.packet_queue, &packet, portMAX_DELAY)) {
        // 直接转换并处理
        uint8_t* data_ptr = (uint8_t*)packet.data;
        data_ptr[packet.length] = '\0';  // 就地修改
        
        // 作为字符串处理
        safe_printf("%s\n", (char*)data_ptr);
        
        // Lua执行
        // lua_execute_command(L, (char*)data_ptr);

        // 新提示符
        safe_printf("\033[36mLua Shell> \033[0m");
    }
  }
  /* USER CODE END LUA_ProcessTask_Handle */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

