#include "st7789.h"
#include "main.h"
#include "spi.h"
#include "stm32h750xx.h"
#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_def.h"
#include "stm32h7xx_hal_spi.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "math.h"

/* 可选的双缓冲内存缓冲区 */
// __attribute__((section(".ram_d1"), aligned(32)))
// static uint16_t double_buffer_memory[2][320*240];

uint16_t ForgColor = 0x0000;
uint16_t BackColor = 0x0000;
pFONT* ASCII_Font = NULL;
pFONT* Chinese_Font = NULL;

__attribute__((section(".ram_d1"), aligned(32)))
uint16_t ST7789_Display_Buffer[1024];

/**
  *	@brief	初始化ST7789显示屏
  *	@note	显示屏初始化为320*240
*/
HAL_StatusTypeDef ST7789_Init(SPI_HandleTypeDef *hspi)
{
    HAL_StatusTypeDef status;
    // 1. 软件复位
    status = WriteCommand(hspi, 0x01); // SWRESET
    HAL_Delay(120);
    
    // 2. 睡眠模式关闭
    status = WriteCommand(hspi, 0x11); // SLPOUT
    HAL_Delay(120);
    
    // 3. 设置颜色模式
    status = WriteCommand(hspi, 0x3A); // COLMOD
    status = WriteData_8bit(hspi, 0x55); // 16位RGB565
    
    // 4. 设置显示方向
    status = WriteCommand(hspi, 0x36); // MADCTL
    status = WriteData_8bit(hspi, 0x00); // 方向设置
    
    // 5. 设置帧率
    status = WriteCommand(hspi, 0xB2); // PORCTRL
    status = WriteData_8bit(hspi, 0x0C);
    status = WriteData_8bit(hspi, 0x0C);
    status = WriteData_8bit(hspi, 0x00);
    status = WriteData_8bit(hspi, 0x33);
    status = WriteData_8bit(hspi, 0x33);
    
    // 6. 门控制
    status = WriteCommand(hspi, 0xB7); // GCTRL
    status = WriteData_8bit(hspi, 0x35);
    
    // 7. VCOMS设置
    status = WriteCommand(hspi, 0xBB); // VCOMS
    status = WriteData_8bit(hspi, 0x19);
    
    // 8. LCM控制
    status = WriteCommand(hspi, 0xC0); // LCMCTRL
    status = WriteData_8bit(hspi, 0x2C);
    
    // 9. VDV和VRH命令使能
    status = WriteCommand(hspi, 0xC2); // VDVVRHEN
    status = WriteData_8bit(hspi, 0x01);
    
    // 10. VRH设置
    status = WriteCommand(hspi, 0xC3); // VRHS
    status = WriteData_8bit(hspi, 0x12);
    
    // 11. VDV设置
    status = WriteCommand(hspi, 0xC4); // VDVS
    status = WriteData_8bit(hspi, 0x20);
    
    // 12. 帧率控制
    status = WriteCommand(hspi, 0xC6); // FRCTRL2
    status = WriteData_8bit(hspi, 0x0F);
    
    // 13. 电源控制
    status = WriteCommand(hspi, 0xD0); // PWCTRL1
    status = WriteData_8bit(hspi, 0xA4);
    status = WriteData_8bit(hspi, 0xA1);
    
    // 14. 正极伽马校正
    status = WriteCommand(hspi, 0xE0); // PVGAMCTRL
    uint8_t pv_gamma[] = {0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 
                          0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23};
    for(int i = 0; i < 14; i++) {
        status = WriteData_8bit(hspi, pv_gamma[i]);
    }
    
    // 15. 负极伽马校正
    status = WriteCommand(hspi, 0xE1); // NVGAMCTRL
    uint8_t nv_gamma[] = {0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F,
                          0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23};
    for(int i = 0; i < 14; i++) {
        status = WriteData_8bit(hspi, nv_gamma[i]);
    }
    
    // 16. 关闭反色显示
    status = WriteCommand(hspi, 0x21); // INVON
    HAL_Delay(10);
    
    // 17. 打开显示
    status = WriteCommand(hspi, 0x29); // DISPON
    HAL_Delay(120);
    
    // 18. 设置显示方向（可选）
    status = WriteCommand(hspi, 0x36); // MADCTL
    status = WriteData_8bit(hspi, 0x70); // 根据需要调整方向

    return status;
}

/**
  * @brief  向ST7789写入命令
  * @param  commands: 命令数组指针
  * @retval HAL状态
  */
HAL_StatusTypeDef WriteCommand(SPI_HandleTypeDef *hspi, uint8_t command)
{
   LCD_DC_Command;
   return HAL_SPI_Transmit(hspi, &command, 1, 1000);
}

/**
  * @brief  向ST7789写入多个命令
  * @param  commands: 命令数组指针
  * @param  len: 命令数量
  * @retval HAL状态
  */
HAL_StatusTypeDef WriteCommands(SPI_HandleTypeDef *hspi, uint8_t *commands, uint16_t len) {
    // 显式类型转换解决编译错误
    uint8_t *buf = (uint8_t*)malloc(len);
    if (buf == NULL) return HAL_ERROR;
    
    LCD_DC_Command;
    memcpy(buf, commands, len);
    
    HAL_StatusTypeDef status = HAL_SPI_Transmit(hspi, buf, len, HAL_MAX_DELAY);
    free(buf);
    return status;
}


/**
  * @brief  向ST7789以8Bits写入数据
  * @param  data: 数据
  * @retval HAL状态
  */
HAL_StatusTypeDef WriteData_8bit(SPI_HandleTypeDef *hspi, uint8_t data)
{
   LCD_DC_Data;
   return HAL_SPI_Transmit(hspi, &data, 1, 1000);
}

/**
  * @brief  向ST7789以16Bits写入数据
  * @param  data: 数据
  * @retval HAL状态
  */
HAL_StatusTypeDef WriteData_16bit(SPI_HandleTypeDef *hspi, uint16_t data)
{
    uint8_t lcd_data_buff[2];
    LCD_DC_Data;
    lcd_data_buff[0] = data>>8;
    lcd_data_buff[1] = data;
    return HAL_SPI_Transmit(hspi, lcd_data_buff, 2, 1000);
}

/**
  * @brief  向ST7789以16Bits写入大量相同数据
  * @param	hspi: SPI句柄
  * @param  pData: 数据
  *	@param	Size: 数据大小
  * @retval HAL状态
  */
HAL_StatusTypeDef SPI_Transmit(SPI_HandleTypeDef *hspi, uint16_t pData, uint32_t Size)
{
   uint32_t    tickstart;  
   uint32_t    Timeout = 1000;
   uint32_t    LCD_pData_32bit;
   uint32_t    LCD_TxDataCount;
   HAL_StatusTypeDef errorcode = HAL_OK;

	/* Check Direction parameter */
	assert_param(IS_SPI_DIRECTION_2LINES_OR_1LINE_2LINES_TXONLY(hspi->Init.Direction));

	/* Process Locked */
	__HAL_LOCK(hspi);

	/* Init tickstart for timeout management*/
	tickstart = HAL_GetTick();

	if(hspi->State != HAL_SPI_STATE_READY)
	{
		errorcode = HAL_BUSY;
		__HAL_UNLOCK(hspi);
		return errorcode;
	}

	if ( Size == 0UL)
	{
		errorcode = HAL_ERROR;
		__HAL_UNLOCK(hspi);
		return errorcode;
	}

	/* Set the transaction information */
	hspi->State       = HAL_SPI_STATE_BUSY_TX;
	hspi->ErrorCode   = HAL_SPI_ERROR_NONE;

	LCD_TxDataCount   = Size;
	LCD_pData_32bit   = (pData<<16)|pData;

	/*Init field not used in handle to zero */
	hspi->pRxBuffPtr  = NULL;
	hspi->RxXferSize  = (uint16_t) 0UL;
	hspi->RxXferCount = (uint16_t) 0UL;
	hspi->TxISR       = NULL;
	hspi->RxISR       = NULL;

	/* Configure communication direction : 1Line */
	if (hspi->Init.Direction == SPI_DIRECTION_1LINE)
	{
		SPI_1LINE_TX(hspi);
	}

	MODIFY_REG(hspi->Instance->CR2, SPI_CR2_TSIZE, 0);

	/* Enable SPI peripheral */
	__HAL_SPI_ENABLE(hspi);

	if (hspi->Init.Mode == SPI_MODE_MASTER)
	{
		 /* Master transfer start */
		 SET_BIT(hspi->Instance->CR1, SPI_CR1_CSTART);
	}

	/* Transmit data in 16 Bit mode */
	while (LCD_TxDataCount > 0UL)
	{
		/* Wait until TXP flag is set to send data */
		if (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_TXP))
		{
			if ((hspi->TxXferCount > 1UL) && (hspi->Init.FifoThreshold > SPI_FIFO_THRESHOLD_01DATA))
			{
				// *((__IO uint32_t *)&hspi->Instance->TXDR) = (uint32_t)LCD_pData_32bit;
        memcpy((void*)&hspi->Instance->TXDR, &LCD_pData_32bit, sizeof(LCD_pData_32bit));
				LCD_TxDataCount -= (uint16_t)2UL;
			}
			else
			{
				// *((__IO uint16_t *)&hspi->Instance->TXDR) = (uint16_t)pData;
        memcpy((void*)&hspi->Instance->TXDR, &pData, sizeof(pData));
				LCD_TxDataCount--;
			}
		}
		else
		{
			/* Timeout management */
			if ((((HAL_GetTick() - tickstart) >=  Timeout) && (Timeout != HAL_MAX_DELAY)) || (Timeout == 0U))
			{
				/* Call standard close procedure with error check */
				SPI_CloseTransfer(hspi);

				/* Process Unlocked */
				__HAL_UNLOCK(hspi);

				SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_TIMEOUT);
				hspi->State = HAL_SPI_STATE_READY;
				return HAL_ERROR;
			}
		}
	}

	if (SPI_WaitOnFlagUntilTimeout(hspi, SPI_SR_TXC, RESET, tickstart, Timeout) != HAL_OK)
	{
		SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_FLAG);
	}

	SET_BIT((hspi)->Instance->CR1 , SPI_CR1_CSUSP);
	/* 等待SPI挂起 */
	if (SPI_WaitOnFlagUntilTimeout(hspi, SPI_FLAG_SUSP, RESET, tickstart, Timeout) != HAL_OK)
	{
		SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_FLAG);
	}
	SPI_CloseTransfer(hspi);   /* Call standard close procedure with error check */

	SET_BIT((hspi)->Instance->IFCR , SPI_IFCR_SUSPC);


	/* Process Unlocked */
	__HAL_UNLOCK(hspi);

	hspi->State = HAL_SPI_STATE_READY;

	if (hspi->ErrorCode != HAL_SPI_ERROR_NONE)
	{
		return HAL_ERROR;
	}
	return errorcode;
}

/**
  * @brief  SPI接口发送缓冲区域
  * @param  hspi: SPI句柄
  * @param  pData: 缓冲区域指针
  * @param	Size: 缓冲区长度
  */
HAL_StatusTypeDef SPI_TransmitBuffer(SPI_HandleTypeDef *hspi, uint16_t *pData, uint32_t Size)
{
  uint32_t    tickstart;  
  uint32_t    Timeout = 1000;
  uint32_t    LCD_TxDataCount;
  HAL_StatusTypeDef errorcode = HAL_OK;

	/* Check Direction parameter */
	assert_param(IS_SPI_DIRECTION_2LINES_OR_1LINE_2LINES_TXONLY(hspi->Init.Direction));

	/* Process Locked */
	__HAL_LOCK(hspi);

	/* Init tickstart for timeout management*/
	tickstart = HAL_GetTick();

	if (hspi->State != HAL_SPI_STATE_READY)
	{
		errorcode = HAL_BUSY;
		__HAL_UNLOCK(hspi);
		return errorcode;
	}

	if ( Size == 0UL)
	{
		errorcode = HAL_ERROR;
		__HAL_UNLOCK(hspi);
		return errorcode;
	}

	/* Set the transaction information */
	hspi->State       = HAL_SPI_STATE_BUSY_TX;
	hspi->ErrorCode   = HAL_SPI_ERROR_NONE;

	LCD_TxDataCount   = Size;                // 获得需要发送的数据量

	/*Init field not used in handle to zero */
	hspi->pRxBuffPtr  = NULL;
	hspi->RxXferSize  = (uint16_t) 0UL;
	hspi->RxXferCount = (uint16_t) 0UL;
	hspi->TxISR       = NULL;
	hspi->RxISR       = NULL;

	/* Configure communication direction : 1Line */
	if (hspi->Init.Direction == SPI_DIRECTION_1LINE)
	{
		SPI_1LINE_TX(hspi);
	}

	MODIFY_REG(hspi->Instance->CR2, SPI_CR2_TSIZE, 0);

	/* Enable SPI peripheral */
	__HAL_SPI_ENABLE(hspi);

	if (hspi->Init.Mode == SPI_MODE_MASTER)
	{
		 /* Master transfer start */
		 SET_BIT(hspi->Instance->CR1, SPI_CR1_CSTART);
	}

	/* Transmit data in 16 Bit mode */
	while (LCD_TxDataCount > 0UL)
	{
		/* Wait until TXP flag is set to send data */
		if (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_TXP))
		{
			if ((LCD_TxDataCount > 1UL) && (hspi->Init.FifoThreshold > SPI_FIFO_THRESHOLD_01DATA))
			{
				// *((__IO uint32_t *)&hspi->Instance->TXDR) = *((uint32_t *)pData);
        memcpy((void*)&hspi->Instance->TXDR, (uint32_t *)pData, sizeof(pData));
				pData += 2;
				LCD_TxDataCount -= 2;
			}
			else
			{
				// *((__IO uint16_t *)&hspi->Instance->TXDR) = *((uint16_t *)pData);
        memcpy((void*)&hspi->Instance->TXDR, &pData, sizeof(pData));
				pData += 1;
				LCD_TxDataCount--;
			}
		}
		else
		{
			/* Timeout management */
			if ((((HAL_GetTick() - tickstart) >=  Timeout) && (Timeout != HAL_MAX_DELAY)) || (Timeout == 0U))
			{
				/* Call standard close procedure with error check */
				SPI_CloseTransfer(hspi);

				/* Process Unlocked */
				__HAL_UNLOCK(hspi);

				SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_TIMEOUT);
				hspi->State = HAL_SPI_STATE_READY;
				return HAL_ERROR;
			}
		}
	}

	if (SPI_WaitOnFlagUntilTimeout(hspi, SPI_SR_TXC, RESET, tickstart, Timeout) != HAL_OK)
	{
		SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_FLAG);
	}

	SET_BIT((hspi)->Instance->CR1 , SPI_CR1_CSUSP);
	if (SPI_WaitOnFlagUntilTimeout(hspi, SPI_FLAG_SUSP, RESET, tickstart, Timeout) != HAL_OK)
	{
		SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_FLAG);
	}
	SPI_CloseTransfer(hspi);   /* Call standard close procedure with error check */

	SET_BIT((hspi)->Instance->IFCR , SPI_IFCR_SUSPC);


	/* Process Unlocked */
	__HAL_UNLOCK(hspi);

	hspi->State = HAL_SPI_STATE_READY;

	if (hspi->ErrorCode != HAL_SPI_ERROR_NONE)
	{
		return HAL_ERROR;
	}
	return errorcode;
}

/**
  * @brief	ST7789的SPI传输超时判定
  *	@param	hspi: SPI句柄
  *	@param	Flag: SPI状态标识
  *	@param	Status: SPI状态
  *	@param	Tickstart: 时间刻起始
  *	@param	Timeout: 超时时间
  * @retval	HAL状态
*/
HAL_StatusTypeDef SPI_WaitOnFlagUntilTimeout(SPI_HandleTypeDef *hspi, uint32_t Flag, FlagStatus Status,
                                                    uint32_t Tickstart, uint32_t Timeout)
{
   /* Wait until flag is set */
   while ((__HAL_SPI_GET_FLAG(hspi, Flag) ? SET : RESET) == Status)
   {
      /* Check for the Timeout */
      if ((((HAL_GetTick() - Tickstart) >=  Timeout) && (Timeout != HAL_MAX_DELAY)) || (Timeout == 0U))
      {
         return HAL_TIMEOUT;
      }
   }
   return HAL_OK;
}

#ifndef BDMA

/**
  * @brief  向ST7789以16Bits和DMA方式写入大量相同数据
  * @param	hspi: SPI句柄
  * @param  pData: 数据
  *	@param	Size: 数据大小
  * @retval HAL状态
  */
HAL_StatusTypeDef SPI_Transmit_DMA(SPI_HandleTypeDef* hspi, uint16_t pData, uint32_t Size)
{
    if (hspi == nullptr || Size == 0) {
        return HAL_ERROR;
    }
    
    // 小数据量使用轮询模式
    if (Size < 32) {
        for (uint32_t i = 0; i < Size; i++) {
            WriteData_16bit(pData);
        }
        return HAL_OK;
    }
    
    HAL_StatusTypeDef final_status = HAL_OK;
    uint32_t remaining = Size;
    
    // 分段传输，每次最多使用整个缓冲区
    while (remaining > 0) {
        uint32_t chunk_size = (remaining > MAX_BUFFER_SIZE) ? MAX_BUFFER_SIZE : remaining;
        
        // 用目标颜色填充缓冲区
        for (uint32_t i = 0; i < chunk_size; i++) {
            ST7789_Display_Buffer[i] = pData;
        }
        
        // 清理缓存（DMA需要）
        SCB_CleanDCache_by_Addr((uint32_t*)ST7789_Display_Buffer, chunk_size * sizeof(uint16_t));
        
        // 启动DMA传输
        HAL_StatusTypeDef status = HAL_SPI_Transmit_DMA(hspi, (uint8_t*)ST7789_Display_Buffer, chunk_size * 2);
        
        if (status != HAL_OK) {
            final_status = status;
            break;
        }
        
        // 等待传输完成
        status = SPI_WaitForDMAComplete(hspi, 1000);
        if (status != HAL_OK) {
            final_status = status;
            break;
        }
        
        remaining -= chunk_size;
    }
    
    return final_status;
}

/**
  * @brief  使用DMA方式通过SPI接口发送缓冲区域
  * @param  hspi: SPI句柄
  * @param  pData: 缓冲区域指针
  * @param	Size: 缓冲区长度
  */
HAL_StatusTypeDef SPI_TransmitBuffer_DMA(SPI_HandleTypeDef* hspi, uint16_t* pData, uint32_t Size)
{
    // 检查参数
    if (hspi == nullptr || pData == nullptr || Size == 0) {
        return HAL_ERROR;
    }
    
    // 如果是小数据量，使用轮询模式
    if (Size < 32) {
        // Size 是像素数，每个像素 2 字节
        return HAL_SPI_Transmit(hspi, (uint8_t*)pData, Size * 2, 1000);
    }
    
    // 设置 SPI 为 16 位模式（如果还没设置）
    hspi->Init.DataSize = SPI_DATASIZE_16BIT;
    if (HAL_SPI_Init(hspi) != HAL_OK) {
        return HAL_ERROR;
    }
    
    // 计算字节数（每个 RGB565 像素 2 字节）
    uint32_t byte_count = Size * 2;
    
    // 清理缓存
    SCB_CleanDCache_by_Addr((uint32_t*)pData, byte_count);
    
    // 启动 DMA 传输
    // 注意：pData 是 uint16_t*，但 HAL_SPI_Transmit_DMA 需要 uint8_t*
    // 直接转换指针类型，但数据本身是 16 位的
    HAL_StatusTypeDef status = HAL_SPI_Transmit_DMA(hspi, (uint8_t*)pData, byte_count);
    
    if (status != HAL_OK) {
        // 恢复 8 位模式
        hspi->Init.DataSize = SPI_DATASIZE_8BIT;
        HAL_SPI_Init(hspi);
        return status;
    }
    
    // 等待传输完成
    status = SPI_WaitForDMAComplete(hspi, 1000);
    
    // 恢复 8 位模式
    hspi->Init.DataSize = SPI_DATASIZE_8BIT;
    HAL_SPI_Init(hspi);
    
    return status;
}

/**
  * @brief  辅助函数:等待DMA传输完成
  * @param  hspi: SPI句柄
  * @param  timeout: 超时时间
  */
HAL_StatusTypeDef SPI_WaitForDMAComplete(SPI_HandleTypeDef* hspi, uint32_t timeout)
{
    uint32_t tickstart = HAL_GetTick();
    
    // 等待 SPI 不再繁忙
    while (hspi->State == HAL_SPI_STATE_BUSY_TX) {
        if (timeout != HAL_MAX_DELAY) {
            if ((HAL_GetTick() - tickstart) >= timeout) {
                // 超时，强制停止 DMA
                HAL_SPI_DMAStop(hspi);
                hspi->State = HAL_SPI_STATE_READY;
                return HAL_TIMEOUT;
            }
        }
    }

    // 检查SPI传输完成标志
    if (!__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_TXC)) {
        printf("SPI transfer not complete! SR: 0x%08lX\n", (unsigned long)hspi->Instance->SR);
        return HAL_ERROR;
    }
    
    // 检查是否有错误
    if (hspi->ErrorCode != HAL_SPI_ERROR_NONE) {
        return HAL_ERROR;
    }
    
    return HAL_OK;
}
#endif

/**
  * @brief	关闭ST7789的SPI发送
  *	@param	hspi: SPI句柄
*/
void SPI_CloseTransfer(SPI_HandleTypeDef *hspi)
{
  uint32_t itflag = hspi->Instance->SR;

  __HAL_SPI_CLEAR_EOTFLAG(hspi);
  __HAL_SPI_CLEAR_TXTFFLAG(hspi);

  /* Disable SPI peripheral */
  __HAL_SPI_DISABLE(hspi);

  /* Disable ITs */
  __HAL_SPI_DISABLE_IT(hspi, (SPI_IT_EOT | SPI_IT_TXP | SPI_IT_RXP | SPI_IT_DXP | SPI_IT_UDR | SPI_IT_OVR | SPI_IT_FRE | SPI_IT_MODF));

  /* Disable Tx DMA Request */
  CLEAR_BIT(hspi->Instance->CFG1, SPI_CFG1_TXDMAEN | SPI_CFG1_RXDMAEN);

  /* Report UnderRun error for non RX Only communication */
  if (hspi->State != HAL_SPI_STATE_BUSY_RX)
  {
    if ((itflag & SPI_FLAG_UDR) != 0UL)
    {
      SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_UDR);
      __HAL_SPI_CLEAR_UDRFLAG(hspi);
    }
  }

  /* Report OverRun error for non TX Only communication */
  if (hspi->State != HAL_SPI_STATE_BUSY_TX)
  {
    if ((itflag & SPI_FLAG_OVR) != 0UL)
    {
      SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_OVR);
      __HAL_SPI_CLEAR_OVRFLAG(hspi);
    }
  }

  /* SPI Mode Fault error interrupt occurred -------------------------------*/
  if ((itflag & SPI_FLAG_MODF) != 0UL)
  {
    SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_MODF);
    __HAL_SPI_CLEAR_MODFFLAG(hspi);
  }

  /* SPI Frame error interrupt occurred ------------------------------------*/
  if ((itflag & SPI_FLAG_FRE) != 0UL)
  {
    SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_FRE);
    __HAL_SPI_CLEAR_FREFLAG(hspi);
  }

  hspi->TxXferCount = (uint16_t)0UL;
  hspi->RxXferCount = (uint16_t)0UL;
}

/**
  * @brief	向ST7789写数据缓冲区
  * @param	DataBuff: 数据缓冲区地址
  * @param	DataSize: 数据缓冲区长度
*/
void WriteBuff(SPI_HandleTypeDef *hspi, uint16_t *DataBuff, uint16_t DataSize)
{
	LCD_DC_Data;
  hspi->Init.DataSize = SPI_DATASIZE_16BIT;
  HAL_SPI_Init(hspi);		
	
	// HAL_SPI_Transmit(hspi, (uint8_t *)DataBuff, DataSize, 1000);
  SPI_TransmitBuffer(hspi, (uint16_t*)DataBuff, DataSize);
	
  hspi->Init.DataSize 	= SPI_DATASIZE_8BIT;
  HAL_SPI_Init(hspi);	
}

/**
  * @brief	设置ST7789的显示地址
  * @param	x1:	X坐标起始点
  * @param	y1:	Y坐标起始点
  * @param	x2:	X坐标结束点
  * @param	y2:	Y坐标结束点
*/
void SetAddress(SPI_HandleTypeDef *hspi, uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2)		
{
	WriteCommand(hspi, 0x2a);
	WriteData_16bit(hspi, x1);
	WriteData_16bit(hspi, x2);

	WriteCommand(hspi, 0x2b);
	WriteData_16bit(hspi, y1);
	WriteData_16bit(hspi, y2);

	WriteCommand(hspi, 0x2c);
}

/**
  * @brief	设置ST7789的颜色
  *	@param	Color: 目标颜色
*/
void ST7789_SetColor(uint32_t Color)
{
	uint16_t Red_Value = 0, Green_Value = 0, Blue_Value = 0;
	Red_Value   = (uint16_t)((Color&0x00F80000)>>8);
	Green_Value = (uint16_t)((Color&0x0000FC00)>>5);
	Blue_Value  = (uint16_t)((Color&0x000000F8)>>3);
	ForgColor 	= (uint16_t)(Red_Value | Green_Value | Blue_Value);
}

/**
  * @brief	设置ST7789的背景颜色
  *	@param	Color: 目标背景颜色
*/
void ST7789_SetBackColor(uint32_t Color)
{
	uint16_t Red_Value = 0, Green_Value = 0, Blue_Value = 0;
	Red_Value   = (uint16_t)((Color&0x00F80000)>>8);
	Green_Value = (uint16_t)((Color&0x0000FC00)>>5);
	Blue_Value  = (uint16_t)((Color&0x000000F8)>>3);
	BackColor 	= (uint16_t)(Red_Value | Green_Value | Blue_Value);  	
}

/**
  * @brief  设置ST7789的字体
  */
void ST7789_SetFont(pFONT *font)
{
  switch (font->FontType)
  {
    // ASCII Font
    case FONT_TYPE_ASCII:
    ASCII_Font = font;
    break;

    // Chinese Font
    case FONT_TYPE_GBK:
    Chinese_Font = font;
    break;
  }
}

/**
  * @brief	清空ST7789显示的内容
*/
void ST7789_Clear(SPI_HandleTypeDef *hspi)
{
	SetAddress(hspi,0,0,320-1,240-1);
	
	LCD_DC_Data;	
	hspi->Init.DataSize = SPI_DATASIZE_16BIT;
	HAL_SPI_Init(hspi);		
	
	SPI_Transmit(hspi, BackColor, 320 * 240);

	hspi->Init.DataSize = SPI_DATASIZE_8BIT;
	HAL_SPI_Init(hspi);
}

/**
  * @brief	在ST7789填充矩形
  *	@param	x: X坐标起始点
  *	@param	y: Y坐标起始点
  *	@param	width: 目标矩形的宽度
  * @param	height:	目标矩形的高度
*/
void ST7789_FillRect(SPI_HandleTypeDef *hspi, uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
  	SetAddress(hspi, x, y, x+width-1, y+height-1);
	  LCD_DC_Data;
  	hspi->Init.DataSize = SPI_DATASIZE_16BIT;
  	HAL_SPI_Init(hspi);
  	SPI_Transmit(hspi, ForgColor, width * height);
	// printf("%x\n",ForgColor);

	  hspi->Init.DataSize = SPI_DATASIZE_8BIT;
  	HAL_SPI_Init(hspi);
}

/**
  * @brief	在ST7789绘制单色图像
  *	@param	x: X坐标起始点
  *	@param	y: Y坐标起始点
  *	@param	width: 目标图像的宽度
  * @param	height:	目标图像的高度
  *	@param	pImage: 目标图像的数组
*/
void ST7789_DrawImage(SPI_HandleTypeDef *hspi, uint16_t x,uint16_t y,uint16_t width,uint16_t height,const uint8_t *pImage)
{  
   uint8_t   disChar;
   uint16_t  Xaddress = x;
   uint16_t  Yaddress = y;
   uint16_t  i=0,j=0,m=0;
   uint16_t  BuffCount = 0;
   uint16_t  Buff_Height = 0;

   Buff_Height = (sizeof(ST7789_Display_Buffer)/2) / height;

	for(i = 0; i <height; i++)
	{
		for(j = 0; j <(float)width/8; j++)  
		{
			disChar = *pImage;

			for(m = 0; m < 8; m++)
			{ 
				if(disChar & 0x01)	
				{		
          ST7789_Display_Buffer[BuffCount] =  ForgColor;
				}
				else		
				{		
					ST7789_Display_Buffer[BuffCount] = BackColor;
				}
				disChar >>= 1;
				Xaddress++;
				BuffCount++;  
				if( (Xaddress - x)==width)
				{											 
					Xaddress = x;				                 
					break;
				}
			}	
			pImage++;			
		}
      if( BuffCount == Buff_Height*width)
      {
         BuffCount = 0;
         SetAddress(hspi, x, Yaddress , x+width-1, Yaddress+Buff_Height-1);
         WriteBuff(hspi, ST7789_Display_Buffer,width*Buff_Height);

         Yaddress = Yaddress+Buff_Height;
      }     
      if( (i+1)== height )
      {
         SetAddress(hspi, x, Yaddress , x+width-1,i+y);
         WriteBuff(hspi, ST7789_Display_Buffer,width*(i+1+y-Yaddress)); 
      }
	}	
}

/**
  * @brief	向ST7789复制缓冲区
  *	@param	x: X坐标起始点
  *	@param	y: Y坐标起始点
  *	@param	width: 目标缓冲区的宽度
  * @param	height:	目标缓冲区的高度
  *	@param	DataBuff: 目标缓冲区的地址
  * @note	把缓冲区指向你的全彩图像数组地址即可显示全彩图像
*/
void ST7789_CopyBuffer(SPI_HandleTypeDef *hspi, uint16_t x, uint16_t y,uint16_t width,uint16_t height,uint16_t *DataBuff)
{
	SetAddress(hspi, x,y,x+width-1,y+height-1);

	LCD_DC_Data;
  hspi->Init.DataSize = SPI_DATASIZE_16BIT;
  HAL_SPI_Init(hspi);		
	
  SPI_TransmitBuffer(hspi, DataBuff,width * height);
	// SPI_TransmitBuffer_DMA(hspi, DataBuff,width * height);
	
//	HAL_SPI_Transmit(&hspi5, (uint8_t *)DataBuff, (x2-x1+1) * (y2-y1+1), 1000) ;
  hspi->Init.DataSize = SPI_DATASIZE_8BIT;
  HAL_SPI_Init(hspi);		
}

/**
  * @brief  绘制一个字符到ST7789屏幕
  * @param  x: X坐标 (0-319)
  * @param  y: Y坐标 (0-239) 
  * @param  ch: 要绘制的字符
  */
void ST7789_DrawChar(SPI_HandleTypeDef *hspi, uint16_t x, uint16_t y, char ch)
{
  if (ASCII_Font == NULL || ch == 0) return;
    // 检查边界
    if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT) return;
    if (x + ASCII_Font->Width > ST7789_WIDTH || y + ASCII_Font->Height > ST7789_HEIGHT) return;
    
    // 计算字符在字体数组中的偏移量
    uint16_t char_offset = (ch - 32) * ASCII_Font->Sizes, i = 0;
    
    // 计算每行需要的字节数
    uint8_t bytes_per_row = (ASCII_Font->Width + 7) / 8;
    
    LCD_DC_Data;
    
    // 根据取模设置：阴码、逐行式、取模走向顺向(高位在前)
    for (uint8_t row = 0; row < ASCII_Font->Height; row++) {
        for (uint8_t byte_idx = 0; byte_idx < bytes_per_row; byte_idx++) {
            uint8_t line_byte = ASCII_Font->pTable[char_offset + row * bytes_per_row + byte_idx];
            uint8_t start_col = byte_idx * 8;
            
            for (uint8_t bit = 0; bit < 8; bit++) {
                uint8_t col = start_col + bit;
                if (col >= ASCII_Font->Width) break;
                
                // 阴码：1表示点亮像素，高位在前
                if (line_byte & (1 << (7 - bit))) {
                    ST7789_Display_Buffer[i] = ForgColor;  // 前景色
                } else {
                    ST7789_Display_Buffer[i] = BackColor;  // 背景色
                }
                i++;
            }
        }
    }

    // 设置字符显示区域
    SetAddress(hspi, x, y, x + ASCII_Font->Width - 1, y + ASCII_Font->Height - 1);
    WriteBuff(hspi, ST7789_Display_Buffer, ASCII_Font->Width * ASCII_Font->Height);
    
    // 切换回8位模式
    // hspi->Init.DataSize = SPI_DATASIZE_8BIT;
    // HAL_SPI_Init(hspi);
}

/**
  * @brief  绘制字符串到ST7789屏幕
  * @param  x: 起始X坐标 (0-319)
  * @param  y: 起始Y坐标 (0-239)
  * @param  str: 要绘制的字符串
  */
void ST7789_DrawString(SPI_HandleTypeDef *hspi, uint16_t x, uint16_t y, const char* str)
{
  if (ASCII_Font == NULL || str == NULL) return;
    uint16_t pos_x = x;
    uint16_t pos_y = y;
    
    while (*str) {
        // 处理换行符
        if (*str == '\n') {
            pos_x = x;
            pos_y += ASCII_Font->Height;
            str++;
            continue;
        }
        
        // 检查是否超出屏幕右边界
        if (pos_x + 8 > ST7789_WIDTH) {
            pos_x = x;
            pos_y += ASCII_Font->Height;
            
            // 检查是否超出屏幕下边界
            if (pos_y + ASCII_Font->Height > ST7789_HEIGHT) break;
        }
        
        // 绘制字符
        ST7789_DrawChar(hspi, pos_x, pos_y, *str);
        
        pos_x += ASCII_Font->Width; // 移动到下一个字符位置
        str++;
    }
}

/**
  * @brief  绘制字符串到ST7789屏幕
  * @param  x: 起始X坐标 (0-239)
  * @param  y: 起始Y坐标 (0-319)
  * @param  str: 要绘制的字符串
*/
void ST7789_DrawNumber(SPI_HandleTypeDef *hspi, uint8_t x, uint8_t y, int32_t num)
{
    char buffer[12];
    snprintf(buffer, sizeof(buffer), "%ld", (long)num);
    ST7789_DrawString(hspi, x, y, buffer);
}

/**
  * @brief  绘制浮点数
  * @param  x: X坐标
  * @param  y: Y坐标
  * @param  num: 要绘制的浮点数
  * @param  decimals: 小数位数
  */
void ST7789_DrawFloat(SPI_HandleTypeDef *hspi, uint16_t x, uint16_t y, float decimals, uint8_t len, uint8_t decs)
{
    char buffer[20];
    
    #if USE_DECIMALS_DISPLAY_FILL_ZERO
    snprintf(buffer, sizeof(buffer), "%0*.*lf", len, decs, decimals);
    #else
    snprintf(buffer, sizeof(buffer), "%*.*lf", len, decs, decimals);
    #endif
    
    ST7789_DrawString(hspi, x, y, buffer);
}

/**
 * @brief 显示单个中文字符
 * @param x: 字符左上角X坐标
 * @param y: 字符左上角Y坐标
 * @param ch: 中文字符
 */
void ST7789_DrawChineseChar(SPI_HandleTypeDef *hspi, uint16_t x, uint16_t y, const char* ch)
{
    if (ch == NULL || Chinese_Font == NULL) return;
    
    // 字符编码到字库索引的映射
    uint16_t char_index = 0;
    uint16_t buffer_index = 0;

    while(1)
	  {		
		  // 通过对比数组中的汉字编码，定位字模地址
		  if ( *(Chinese_Font->pTable + (buffer_index+1)*Chinese_Font->Sizes + 0)==*ch && *(Chinese_Font->pTable + (buffer_index+1)*Chinese_Font->Sizes + 1)==*(ch+1) )	
		  {
			  char_index=buffer_index;	// 字模地址偏移
			  break;
		  }				
		  buffer_index+=2;	// 每个中文字符占两字节

		  if(buffer_index >= Chinese_Font->Table_Rows)	return;	// 字模列表没这个字
	  }
    buffer_index = 0;
    // 计算字模数据偏移量
    const uint8_t* char_data = Chinese_Font->pTable + char_index * Chinese_Font->Sizes;
    
    // 边界检查
    if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT) return;
    if (x + Chinese_Font->Width > ST7789_WIDTH || y + Chinese_Font->Height > ST7789_HEIGHT) return;
    
    // 计算每行需要的字节数
    uint8_t bytes_per_row = (Chinese_Font->Width + 7) / 8;
    
    LCD_DC_Data;
    
    // 清空显示缓冲区
    memset(ST7789_Display_Buffer, 0, sizeof(ST7789_Display_Buffer));
    
    // 遍历每一行
    for(uint8_t row = 0; row < Chinese_Font->Height; row++) {
        // 遍历每行的每个字节
        for(uint8_t byte_idx = 0; byte_idx < bytes_per_row; byte_idx++) {
            uint8_t line_byte = char_data[row * bytes_per_row + byte_idx];
            uint8_t start_col = byte_idx * 8;
            
            // 遍历每个字节的每个位
            for(uint8_t bit = 0; bit < 8; bit++) {
                uint8_t col = start_col + bit;
                if (col >= Chinese_Font->Width) break;
                
                // 检查像素点是否应该点亮（阴码，高位在前）
                if (line_byte & (1 << (7 - bit))) {
                    ST7789_Display_Buffer[buffer_index] = ForgColor;  // 前景色
                } else {
                    ST7789_Display_Buffer[buffer_index] = BackColor;  // 背景色
                }
                buffer_index++;
            }
        }
    }
    
    // 设置显示区域并写入数据
    SetAddress(hspi, x, y, x + Chinese_Font->Width - 1, y + Chinese_Font->Height - 1);
    WriteBuff(hspi, ST7789_Display_Buffer, Chinese_Font->Width * Chinese_Font->Height);
}

/**
 * @brief 显示中文字符串
 * @param x: 字符串起始X坐标
 * @param y: 字符串起始Y坐标
 * @param str: 中文字符串
 * @note 可以在字符间添加像素间隔，修改 spacing 参数即可
 */
void ST7789_DrawChineseString(SPI_HandleTypeDef *hspi, uint16_t x, uint16_t y, const char* str)
{
    if (str == NULL || Chinese_Font == NULL) return;
    
    uint16_t currentX = x;
    uint16_t currentY = y;
    
    // 字符间像素间隔，可以调整这个值来增加或减少字符间距
    uint8_t spacing = 0; // 可以修改为 1, 2 等值来增加间距
    
    while (*str != '\0') {
        // 检查是否是中文字符（GB2312编码范围）
        if ((uint8_t)*str >= 0xA1 && (uint8_t)*str <= 0xF7) {
            // 确保有完整的双字节
            if (*(str + 1) == '\0') break;
            
            // 显示中文字符
            ST7789_DrawChineseChar(hspi, currentX, currentY, str);
            
            // 移动到下一个字符位置
            currentX += Chinese_Font->Width + spacing; // 字符宽度 + 间距
            
            str += 2; // 跳过2个字节（中文字符）
        } 
        else if ((uint8_t)*str >= 0x20 && (uint8_t)*str <= 0x7E) {
            // ASCII字符 - 使用现有的DrawChar函数
            ST7789_DrawChar(hspi, currentX, currentY, *str);
            currentX += ASCII_Font->Width + spacing; // 字符宽度 + 间距
            
            str += 1; // 跳过1个字节（ASCII字符）
        }
        else {
            // 其他字符跳过
            str += 1;
        }
        
        // 换行检查
        if (currentX + fmax(Chinese_Font->Width, ASCII_Font->Width) > ST7789_WIDTH) {
            currentX = x;
            currentY += fmax(Chinese_Font->Height, ASCII_Font->Height) + 1; // 行间距
            
            if (currentY + fmax(Chinese_Font->Height, ASCII_Font->Height) > ST7789_HEIGHT) {
                break;
            }
        }
    }
}
