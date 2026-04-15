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
#include "sdmmc.h"
#include "terminal.h"
#include "uart_dyn_rx.h"
#include <stdlib.h>
#include <string.h>
#include <sys/_intsup.h>

#include "lua.h"
#include "lualib.h"
#include "portable.h"
#include "setjmp.h"
#include "fatfs.h"
#include "config.h"
// #include "embedded_lua.h"
#include "hardware_bindings.h"
#include "lua_file_execute.h"
#include "lua_fatfs.h"
#include "lua_display.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define AUTO_MOUNT 1
#define AUTO_INIT_DISPLAY 1
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// #define DTCMRAM_ADDR       ((uint8_t*)0x20000000)
// #define DTCMRAM_SIZE       ((uint32_t)0x00010000)
#define AXIRAM_ADDR        ((uint8_t*)0x24002000)
#define AXIRAM_SIZE        ((uint32_t)0x00078000)
#define SDRAM_ADDR         ((uint8_t*)0xC0000000)
#define SDRAM_SIZE         ((uint32_t)0x02000000)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
// static jmp_buf g_lua_panic_jmp;

static HeapRegion_t HeapRAMRegions[]=
{
  // {DTCMRAM_ADDR, DTCMRAM_SIZE},
  {AXIRAM_ADDR, AXIRAM_SIZE},
  {SDRAM_ADDR, SDRAM_SIZE},
  {NULL,0}
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
  .stack_size = 2048 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
int lua_panic_handler(lua_State* L);
int safe_lua_execute(lua_State* L, const char* code, const char* cmd_name);

void FatFs_GetVolume(void);
void FatFS_FileList(char* path);
uint8_t FatFs_FileTest(void);
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
  // safe_printf("\rThread run\n");
  void *axi_sram_ptr = pvPortMalloc(100);
  if(axi_sram_ptr) {
      safe_printf("\r\033[35mSRAM allocation test: PASS (0x%lX)\033[0m\r\n", axi_sram_ptr);
      vPortFree(axi_sram_ptr);
  } else {
      safe_printf("\r\033[31mSRAM allocation test: FAIL\033[0m\r\n");
  }
  void *sdram_ptr = pvPortMalloc(1024 * 1024);  // 1MB
  if(sdram_ptr) {
      safe_printf("\r\033[35mSDRAM allocation test: PASS (0x%lX)\033[0m\r\n", sdram_ptr);
      
      /* 验证确实在 SDRAM 地址范围内 */
      if((uint32_t)sdram_ptr >= (uint32_t)SDRAM_ADDR && 
         (uint32_t)sdram_ptr < (uint32_t)SDRAM_ADDR + SDRAM_SIZE) {
          safe_printf("\r\033[35mSDRAM address verification: PASS (0x%lX)\033[0m\r\n", sdram_ptr);
      } else {
          safe_printf("\r\033[35mSDRAM address verification: FAIL (0x%lX)\033[0m\r\n", sdram_ptr);
      }
      vPortFree(sdram_ptr);
  } else {
      safe_printf("\r\033[31mSDRAM allocation test: FAIL\033[0m\r\n");
  }
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
  // UART_DataPacket_t packet;
  char* received_cmd = NULL;
  lua_State* L;
  // 创建Lua虚拟机（这个任务的私有资源）
  L = luaL_newstate();
  if (L == NULL) {
    vTaskDelete(NULL);
  }
  luaL_openlibs(L); // 打开基础库
  // embedded_lua_init();
  hardware_bindings_init(L); // 注册你的硬件API
  fatfs_bindings_init(L);
  display_bindings_init(L);
  #if AUTO_INIT_DISPLAY
  lua_display_init(L);
  #endif
  #if AUTO_MOUNT
  if(lua_fatfs_mount(L) == 0)
    lua_auto_execute_startup(L);
    // FatFs_FileTest();
  #endif
  Terminal_Init();
  // UART_Dynamic_Receive_Init();
  // safe_printf("\033[36mLua Shell> \033[0m");
  /* Infinite loop */
  for(;;)
  {
    //  if (xQueueReceive(UART_Receiver.packet_queue, &packet, portMAX_DELAY)) {
    //     // 直接转换并处理
    //     uint8_t* data_ptr = (uint8_t*)packet.data;
    //     data_ptr[packet.length] = '\0';  // 就地修改
        
    //     // 作为字符串处理
    //     safe_printf("%s\n", (char*)data_ptr);
        
    //     // Lua执行
    //     // lua_execute_command(L, (char*)data_ptr);

    //     // 新提示符
    //     safe_printf("\033[36mLua Shell> \033[0m");
    // }

    if (xQueueReceive(g_terminal.cmd_queue, &received_cmd, portMAX_DELAY)) {
      if (received_cmd != NULL) {
        // 打印原命令，调试可用
        // safe_printf("\r\nReceived command: %s\r\n", received_cmd);
        
        // TODO: 在这里添加命令解析和处理逻辑
        // 执行命令
        if(safe_lua_execute(L, received_cmd, received_cmd) == LUA_OK){
          safe_printf("\033[32m\r%s: Operation Sucess End\033[0m\r\n", received_cmd);
        }
        
        // 释放内存
        free(received_cmd);
        received_cmd = NULL;
        // 显示新的提示符
        safe_printf("\r\033[36m[Lua Shell]>\033[0m ");
      }
    }
    // osDelay(1);
  }
  /* USER CODE END LUA_ProcessTask_Handle */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

//	函数：FatFs_FileTest
//	功能：进行文件写入和读取测试
//
uint8_t FatFs_FileTest(void)	//文件创建和写入测试
{
	uint8_t i = 0;
	uint16_t BufferSize = 0;
	FIL	MyFile;			// 文件对象
	UINT 	MyFile_Num;		//	数据长度
	BYTE 	MyFile_WriteBuffer[] = "STM32H750XBH6 SD卡 文件系统测试";	//要写入的数据
	BYTE 	MyFile_ReadBuffer[1024];	//要读出的数据
  uint8_t MyFile_Res;

	safe_printf("-------------FatFs File Create and Read/Write Test---------------\r\n");

	if(f_open(&MyFile,"0:FatFs Test.txt",FA_CREATE_ALWAYS | FA_WRITE) == FR_OK) //打开文件，若不存在则创建该文件
	{
		safe_printf("File Open/Create Sucess,Ready Write Data...\r\n");

		if (f_write(&MyFile,MyFile_WriteBuffer,sizeof(MyFile_WriteBuffer),&MyFile_Num) == FR_OK)	//向文件写入数据
		{
			safe_printf("Write Sucess,Write Content\r\n");
			safe_printf("%s\r\n",MyFile_WriteBuffer);
		}
		else
		{
			safe_printf("Failed to write file. Please check the SD card or reformat it!\r\n");
			f_close(&MyFile);	  //关闭文件
			return ERROR;
		}
		f_close(&MyFile);	  //关闭文件
	}
	else
	{
		safe_printf("Unable to open/create file, please check the SD card or reformat it!\r\n");
		f_close(&MyFile);	  //关闭文件
		return ERROR;
	}

	osDelay(1);
	safe_printf("-------------FatFs File Read Test---------------\r\n");

	BufferSize = sizeof(MyFile_WriteBuffer)/sizeof(BYTE);									// 计算写入的数据长度
	MyFile_Res = f_open(&MyFile,"0:FatFs Test.txt",FA_OPEN_EXISTING | FA_READ);	//打开文件，若不存在则创建该文件
	MyFile_Res = f_lseek(&MyFile, SEEK_SET);									//移动文件指针到开头
	MyFile_Res = f_read(&MyFile,MyFile_ReadBuffer,BufferSize,&MyFile_Num);		// 读取文件
	if(MyFile_Res == FR_OK)
	{
		safe_printf("File read successfully, verifying data...\r\n");

		for(i=0;i<BufferSize;i++)
		{
			if(MyFile_WriteBuffer[i] != MyFile_ReadBuffer[i])		// 校验数据
			{
				safe_printf("Verification failed, please check the SD card or reformat it!\r\nContent written on the card:%s\nContent read:%s\n",MyFile_WriteBuffer,MyFile_ReadBuffer);
				f_close(&MyFile);	  //关闭文件
				return ERROR;
			}
		}
		safe_printf("Verification successful, the data read is:\r\n");
		safe_printf("%s\r\n",MyFile_ReadBuffer);
	}
	else
	{
		safe_printf("Unable to read the file. Please check the SD card or reformat it!\r\n");
		f_close(&MyFile);	  //关闭文件
		return ERROR;
	}

	f_close(&MyFile);	  //关闭文件
	return SUCCESS;
}
/* USER CODE END Application */

