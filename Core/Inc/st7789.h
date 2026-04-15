#ifndef ST7789_H
#define ST7789_H

#include "main.h"
#include "stdint.h"
#include "stm32h7xx_hal_spi.h"
#include "stm32h7xx_hal_def.h"
#include "display_font.h"

#ifndef USE_DECIMALS_DISPLAY_FILL_ZERO
#define USE_DECIMALS_DISPLAY_FILL_ZERO 0
#endif

    #define ST7789_WIDTH  320
    #define ST7789_HEIGHT 240
    static const uint32_t BUFFER_SIZE = ST7789_WIDTH * ST7789_HEIGHT;

    // 私有SPI传输辅助函数
    HAL_StatusTypeDef WriteCommand(SPI_HandleTypeDef *hspi, uint8_t command);
    HAL_StatusTypeDef WriteCommands(SPI_HandleTypeDef *hspi, uint8_t *commands, uint16_t len);
    HAL_StatusTypeDef WriteData_8bit(SPI_HandleTypeDef *hspi, uint8_t data);
    HAL_StatusTypeDef WriteData_16bit(SPI_HandleTypeDef *hspi, uint16_t data);
    HAL_StatusTypeDef SPI_Transmit(SPI_HandleTypeDef *hspi, uint16_t pData, uint32_t Size);
    HAL_StatusTypeDef SPI_TransmitBuffer (SPI_HandleTypeDef *hspi, uint16_t *pData, uint32_t Size);
    HAL_StatusTypeDef SPI_WaitOnFlagUntilTimeout(SPI_HandleTypeDef *hspi, uint32_t Flag, FlagStatus Status, uint32_t Tickstart, uint32_t Timeout);
    /* 
        在有BDMA时，不许用，因为我踩坑踩进天坑了，BDMA只能够访问AXI SRAM Domain 4 (64KiB)，
        如果你一定要用，可以，换其他单片机，比如F4xx就行，但函数未经完整的测试，
        如有BUG请自行解决后传到代码仓库或者我的群里
    */
    #ifndef BDMA
    static const uint16_t MAX_BUFFER_SIZE = 1024;
    HAL_StatusTypeDef SPI_Transmit_DMA(SPI_HandleTypeDef* hspi, uint16_t pData, uint32_t Size);
    HAL_StatusTypeDef SPI_TransmitBuffer_DMA(SPI_HandleTypeDef* hspi, uint16_t* pData, uint32_t Size);
    HAL_StatusTypeDef SPI_WaitForDMAComplete(SPI_HandleTypeDef* hspi, uint32_t timeout);
    #endif

    void SPI_CloseTransfer(SPI_HandleTypeDef *hspi);
    void WriteBuff(SPI_HandleTypeDef *hspi, uint16_t *DataBuff, uint16_t DataSize);
    void SetAddress(SPI_HandleTypeDef *hspi, uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2);

    // 公开的ST7789操作函数
    HAL_StatusTypeDef ST7789_Init(SPI_HandleTypeDef *hspi);
    void ST7789_SetColor(uint32_t color);
    void ST7789_SetBackColor(uint32_t color);
    void ST7789_SetFont(pFONT *font);
    void ST7789_Clear(SPI_HandleTypeDef *hspi);
    void ST7789_FillRect(SPI_HandleTypeDef *hspi, uint16_t x, uint16_t y, uint16_t width, uint16_t height);
    void ST7789_DrawImage(SPI_HandleTypeDef *hspi, uint16_t x,uint16_t y,uint16_t width,uint16_t height,const uint8_t *pImage);
    void ST7789_CopyBuffer(SPI_HandleTypeDef *hspi, uint16_t x, uint16_t y,uint16_t width,uint16_t height,uint16_t *DataBuff);
    void ST7789_DrawChar(SPI_HandleTypeDef *hspi, uint16_t x, uint16_t y, char ch);
    void ST7789_DrawString(SPI_HandleTypeDef *hspi, uint16_t x, uint16_t y, const char* str);
    void ST7789_DrawNumber(SPI_HandleTypeDef *hspi, uint8_t x, uint8_t y, int32_t num);
    void ST7789_DrawFloat(SPI_HandleTypeDef *hspi, uint16_t x, uint16_t y, float decimals, uint8_t len, uint8_t decs);
    void ST7789_DrawChineseChar(SPI_HandleTypeDef *hspi, uint16_t x, uint16_t y, const char* ch);
    void ST7789_DrawChineseString(SPI_HandleTypeDef *hspi, uint16_t x, uint16_t y, const char* str);

#endif // ST7789_HPP