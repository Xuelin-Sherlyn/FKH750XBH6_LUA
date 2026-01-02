#ifndef ST7789_C_INTERFACE_H
#define ST7789_C_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "stm32h7xx_hal_spi.h"
#include "display_font.h"  // 包含字体头文件

// // 滚动方向枚举
// typedef enum {
//     SCROLL_LEFT = 0,
//     SCROLL_RIGHT,
//     SCROLL_UP, 
//     SCROLL_DOWN
// } ScrollDirection_t;

// // 图片信息结构体
// typedef struct {
//     uint16_t width;
//     uint16_t height;
//     uint8_t *data;
//     uint32_t data_size;
// } Image_t;

// 不透明的句柄类型（C 代码不知道具体实现）
typedef void* ST7789_Handle;

// ============== 基本操作 ==============
ST7789_Handle ST7789_Create(SPI_HandleTypeDef* hspi);
void ST7789_Destroy(ST7789_Handle handle);
HAL_StatusTypeDef ST7789_InitDisplay(ST7789_Handle handle);

// ============== 颜色和字体设置 ==============
void ST7789_SetColor(ST7789_Handle handle, uint32_t color);
void ST7789_SetBackColor(ST7789_Handle handle, uint32_t color);
void ST7789_SetFont(ST7789_Handle handle, pFONT *font);

// ============== 清屏和填充 ==============
void ST7789_Clear(ST7789_Handle handle);
void ST7789_FillRect(ST7789_Handle handle, uint16_t x, uint16_t y, uint16_t width, uint16_t height);

// ============== 文字绘制 ==============
void ST7789_DrawChar(ST7789_Handle handle, uint16_t x, uint16_t y, char ch);
void ST7789_DrawString(ST7789_Handle handle, uint16_t x, uint16_t y, const char* str);
void ST7789_DrawNumber(ST7789_Handle handle, uint8_t x, uint8_t y, int32_t num);
void ST7789_DrawFloat(ST7789_Handle handle, uint16_t x, uint16_t y, float num, uint8_t len, uint8_t decs);
void ST7789_DrawChineseChar(ST7789_Handle handle, uint16_t x, uint16_t y, const char* ch);
void ST7789_DrawChineseString(ST7789_Handle handle, uint16_t x, uint16_t y, const char* str);

// ============== 图像操作 ==============
void ST7789_DrawImage(ST7789_Handle handle, uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t *pImage);
void ST7789_CopyBuffer(ST7789_Handle handle, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t *DataBuff);

// ============== USB MIDI 专用函数 ==============
void ST7789_ShowStatus(ST7789_Handle handle, const char* status);
void ST7789_ShowMidiMessage(ST7789_Handle handle, uint8_t channel, uint8_t note, uint8_t velocity);
void ST7789_ShowUsbStatus(ST7789_Handle handle, const char* usb_status);

#ifdef __cplusplus
}
#endif

// ============== 简化调用的宏（可选） ==============
// 注意：这些宏只能在 C 代码中使用
#ifndef __cplusplus
#define LCD_INIT(hspi)                  ST7789_Create(hspi)
#define LCD_DEINIT(handle)              ST7789_Destroy(handle)
#define LCD_DISPLAY_INIT(handle)        ST7789_InitDisplay(handle)
#define LCD_CLEAR(handle)               ST7789_Clear(handle)
#define LCD_SET_COLOR(handle, color)    ST7789_SetColor(handle, color)
#define LCD_SET_BG_COLOR(handle, color) ST7789_SetBackColor(handle, color)
#define LCD_SET_FONT(handle, font)      ST7789_SetFont(handle, font)
#define LCD_PRINT(handle, x, y, str)    ST7789_DrawString(handle, x, y, str)
#define LCD_PRINT_NUM(handle, x, y, num) ST7789_DrawNumber(handle, x, y, num)
#define LCD_PRINT_FLOAT(handle, x, y, num, len, dec) ST7789_DrawFloat(handle, x, y, num, len, dec)
#define LCD_SHOW_STATUS(handle, str)    ST7789_ShowStatus(handle, str)
#define LCD_SHOW_MIDI(handle, ch, n, v) ST7789_ShowMidiMessage(handle, ch, n, v)
#define LCD_SHOW_USB(handle, status)    ST7789_ShowUsbStatus(handle, status)
#endif

#endif // ST7789_C_INTERFACE_H