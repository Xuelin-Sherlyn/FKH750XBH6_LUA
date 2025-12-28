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
// #include "embedded_lua.h"
#include "hardware_bindings.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DTCMRAM_ADDR       ((uint8_t*)0x20010000)
#define DTCMRAM_SIZE       ((uint32_t)0x00010000)
#define AXIRAM_ADDR        ((uint8_t*)0x24002000)
#define AXIRAM_SIZE        ((uint32_t)0x00078000)
#define SDRAM_ADDR         ((uint8_t*)0xC0000000)
#define SDRAM_SIZE         ((uint32_t)0x01000000)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
FATFS *fs;
static jmp_buf g_lua_panic_jmp;

static HeapRegion_t HeapRAMRegions[]=
{
  {DTCMRAM_ADDR, DTCMRAM_SIZE},
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
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
int lua_panic_handler(lua_State* L);
int safe_lua_execute(lua_State* L, const char* code, const char* cmd_name);

void FatFs_Check(void);
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
        safe_printf("\rSRAM allocation test: PASS (%p)\r\n", axi_sram_ptr);
        vPortFree(axi_sram_ptr);
    } else {
        safe_printf("\rSRAM allocation test: FAIL\r\n");
    }
    void *sdram_ptr = pvPortMalloc(1024 * 1024);  // 1MB
    if(sdram_ptr) {
        safe_printf("\rSDRAM allocation test: PASS (%p)\r\n", sdram_ptr);
        
        /* 验证确实在 SDRAM 地址范围内 */
        if((uint32_t)sdram_ptr >= (uint32_t)SDRAM_ADDR && 
           (uint32_t)sdram_ptr < (uint32_t)SDRAM_ADDR + SDRAM_SIZE) {
            safe_printf("\rSDRAM address verification: PASS (%p)\r\n", sdram_ptr);
        } else {
            safe_printf("\rSDRAM address verification: FAIL (%p)\r\n", sdram_ptr);
        }
        vPortFree(sdram_ptr);
    } else {
        safe_printf("\rSDRAM allocation test: FAIL\r\n");
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
  FatFs_Check();
  Terminal_Init();
  // 创建Lua虚拟机（这个任务的私有资源）
  L = luaL_newstate();
  if (L == NULL) {
    vTaskDelete(NULL);
  }
  luaL_openlibs(L); // 打开基础库
  // embedded_lua_init();
  hardware_bindings_init(L); // 注册你的硬件API
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
// 安全的Lua初始化
lua_State* safe_lua_init(void) {
    lua_State* L = luaL_newstate();
    if (!L) return NULL;
    
    // 关键：设置紧急处理器
    lua_atpanic(L, lua_panic_handler);
    
    if (setjmp(g_lua_panic_jmp) == 0) {
        // 正常初始化流程
        luaL_openselectedlibs(L, LUA_STRLIBK | LUA_MATHLIBK, 0);
        hardware_bindings_init(L);
        return L;
    } else {
        // 从Panic中恢复：关闭并重新创建Lua状态机
        safe_printf("\033[33mRecreating Lua VM after panic...\033[0m\r\n");
        lua_close(L);
        return safe_lua_init(); // 递归但有限，因为panic应不常发生
    }
}

int safe_lua_execute(lua_State* L, const char* code, const char* cmd_name) {
    // 1. 加载代码（捕获语法错误）
    int load_status = luaL_loadstring(L, code);
    if (load_status != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        safe_printf("\r\n\033[31mSyntax Error: %s\033[0m\r\n", err);
        lua_pop(L, 1);
        return -1;
    }
    
    // 2. 在保护模式下执行（捕获所有运行时错误）
    int exec_status = lua_pcall(L, 0, LUA_MULTRET, 0);
    
    // 3. 处理执行结果
    if (exec_status == LUA_OK) {
        // 成功 - 处理返回值（如果有）
        int nresults = lua_gettop(L);
        if (nresults > 0) {
            safe_printf("\r\n\033[32mResult(s):\033[0m ");
            for (int i = 1; i <= nresults; i++) {
                if (lua_isinteger(L, i)) {
                    safe_printf("%lld ", lua_tointeger(L, i));
                } else if (lua_isstring(L, i)) {
                    safe_printf("%s ", lua_tostring(L, i));
                } else if (lua_isboolean(L, i)) {
                    safe_printf(lua_toboolean(L, i) ? "true " : "false ");
                }
            }
            safe_printf("\r\n");
            lua_pop(L, nresults);
        }
        safe_printf("\r\n\033[32m%s: OK\033[0m\r\n", cmd_name);
        return 0;
    }
    else {
        // 运行时错误（包括调用不存在的函数）
        const char* err = lua_tostring(L, -1);
        const char* err_type = 
            (exec_status == LUA_ERRRUN) ? "Runtime Error" :
            (exec_status == LUA_ERRMEM) ? "Memory Error" :
            (exec_status == LUA_ERRERR) ? "Error Handler Error" : "Unknown Error";
        
        safe_printf("\r\n\033[31m%s: %s\033[0m\r\n", err_type, err);
        lua_pop(L, 1);
        return -2;
    }
}

// Lua紧急错误回调（当发生无法恢复的Lua内部错误时调用）
int lua_panic_handler(lua_State* L) {
    const char* msg = lua_tostring(L, -1);
    if (msg == NULL) msg = "unknown panic";
    safe_printf("\r\n\033[31m[LUA PANIC] %s\033[0m\r\n", msg);
    
    // 跳转到安全恢复点
    longjmp(g_lua_panic_jmp, 1);
    return 0; // 永远不会执行到这里
}

// 判断FatFs是否挂载成功，若没有创建FatFs则格式化SD卡
void FatFs_Check(void)
{
	BYTE work[_MAX_SS];

	FATFS_LinkDriver(&SD_Driver, SDPath);		   // 初始化驱动
	retSD = f_mount(&SDFatFS, SDPath, 1);	 // 挂载SD卡

	if (retSD == FR_OK)	//判断是否挂载成功
	{
		safe_printf("\r\033[32mSD File System Mount Sucess\033[0m\r\n");
	}
	else
	{
		if(retSD == 13)
		{
			safe_printf("\r\033[33mSD Card`s File system has not been created yet, about to format\033[0m\r\n");

			retSD = f_mkfs(SDPath,FM_FAT32,0,work,sizeof work);		//格式化SD卡，FAT32，簇默认大小16K

			if (retSD == FR_OK)		//判断是否格式化成功
				safe_printf("\r\033[32mSD card formatted successfully!\033[0m\r\n");
			else
				safe_printf("\r\033[31mFormatting failed, please check or replace the SD card!\033[0m\r\n");
		}
		else
			safe_printf("\r\033[31mMount Fail:%d\033[0m\r\n",retSD);
	}
}

//	函数：FatFs_GetVolume
//	功能：计算设备的容量，包括总容量和剩余容量
void FatFs_GetVolume(void)	// 计算设备容量
{
//	static FATFS *fs;		//定义结构体指针
	uint32_t SD_CardCapacity = 0;		//SD卡的总容量
	uint32_t SD_FreeCapacity = 0;		//SD卡空闲容量
	DWORD fre_clust, fre_sect, tot_sect; 	//空闲簇，空闲扇区数，总扇区数

	f_getfree(SDPath,&fre_clust,&fs);			//获取SD卡剩余的簇

	tot_sect = (fs->n_fatent-2) * fs->csize;	//总扇区数量 = 总的簇 * 每个簇包含的扇区数
	fre_sect = fre_clust * fs->csize;			//计算剩余的可用扇区数

	SD_CardCapacity = tot_sect / 2048 ;	// SD卡总容量 = 总扇区数 * 512( 每扇区的字节数 ) / 1048576(换算成MB)
	SD_FreeCapacity = fre_sect / 2048 ;	//计算剩余的容量，单位为M
	safe_printf("-------------------Get device capacity information-----------------\r\n");
	safe_printf("SD Capacity:%luMB\r\n", (unsigned long)SD_CardCapacity);
	safe_printf("SD Remaining:%luMB\r\n", (unsigned long)SD_FreeCapacity);
}

// 列出介质内的文件
void FatFS_FileList(char* path)
{
	FRESULT res; //文件操作返回代码
	DIR dir;     //目录对象
	static FILINFO fno; //文件信息句柄

  if(path != NULL)
  {
    safe_printf("Target Media Path:%s\n", path);
	  res = f_opendir(&dir, path); //打开目录
	  if (res == FR_OK)
	  {
		  while (1) //循环读取
		  {
			  res = f_readdir(&dir, &fno);
			  if (res != FR_OK || fno.fname[0] == 0) break;
			  char path_pic[512];
			  sprintf(path_pic, "%s/%s", path, fno.fname);
			  safe_printf("Files Path:%s\r\n", path_pic);
			  osDelay(10);
		  }
		  f_closedir(&dir); //关闭目录
	  }
	  else
	  {
		  safe_printf("open Fail: %d\r\n", res); //输出失败信息
	  }
  }
  else 
  {
    safe_printf("Target Media Path is NULL!\n"); 
  }
}

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

