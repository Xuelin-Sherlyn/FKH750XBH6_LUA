#include "st7789_c_interface.h"
#include "st7789.hpp"
#include <cstdio>
#include <string.h>

// 全局 LCD 实例
static ST7789* g_lcd_instance = nullptr;

extern "C" {
    
// ============== 基本操作 ==============
    
ST7789_Handle ST7789_Create(SPI_HandleTypeDef* hspi)
{
    if (g_lcd_instance != nullptr) {
        // 已经创建，返回现有实例
        return g_lcd_instance;
    }
    
    g_lcd_instance = new ST7789(hspi);
    return g_lcd_instance;
}

void ST7789_Destroy(ST7789_Handle handle)
{
    if (handle != nullptr && handle == g_lcd_instance) {
        delete g_lcd_instance;
        g_lcd_instance = nullptr;
    }
}

HAL_StatusTypeDef ST7789_InitDisplay(ST7789_Handle handle)
{
    if (handle == nullptr) return HAL_ERROR;
    return static_cast<ST7789*>(handle)->Init();
}

// ============== 颜色和字体设置 ==============

void ST7789_SetColor(ST7789_Handle handle, uint32_t color)
{
    if (handle == nullptr) return;
    static_cast<ST7789*>(handle)->SetColor(color);
}

void ST7789_SetBackColor(ST7789_Handle handle, uint32_t color)
{
    if (handle == nullptr) return;
    static_cast<ST7789*>(handle)->SetBackColor(color);
}

void ST7789_SetFont(ST7789_Handle handle, pFONT *font)
{
    if (handle == nullptr || font == nullptr) return;
    static_cast<ST7789*>(handle)->SetFont(font);
}

// ============== 清屏和填充 ==============

void ST7789_Clear(ST7789_Handle handle)
{
    if (handle == nullptr) return;
    static_cast<ST7789*>(handle)->Clear();
}

void ST7789_FillRect(ST7789_Handle handle, uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    if (handle == nullptr) return;
    static_cast<ST7789*>(handle)->FillRect(x, y, width, height);
}

// ============== 文字绘制 ==============

void ST7789_DrawChar(ST7789_Handle handle, uint16_t x, uint16_t y, char ch)
{
    if (handle == nullptr) return;
    static_cast<ST7789*>(handle)->DrawChar(x, y, ch);
}

void ST7789_DrawString(ST7789_Handle handle, uint16_t x, uint16_t y, const char* str)
{
    if (handle == nullptr || str == nullptr) return;
    static_cast<ST7789*>(handle)->DrawString(x, y, str);
}

void ST7789_DrawNumber(ST7789_Handle handle, uint8_t x, uint8_t y, int32_t num)
{
    if (handle == nullptr) return;
    static_cast<ST7789*>(handle)->DrawNumber(x, y, num);
}

void ST7789_DrawFloat(ST7789_Handle handle, uint16_t x, uint16_t y, float num, uint8_t len, uint8_t decs)
{
    if (handle == nullptr) return;
    static_cast<ST7789*>(handle)->DrawFloat(x, y, num, len, decs);
}

void ST7789_DrawChineseChar(ST7789_Handle handle, uint16_t x, uint16_t y, const char* ch)
{
    if (handle == nullptr || ch == nullptr) return;
    static_cast<ST7789*>(handle)->DrawChineseChar(x, y, ch);
}

void ST7789_DrawChineseString(ST7789_Handle handle, uint16_t x, uint16_t y, const char* str)
{
    if (handle == nullptr || str == nullptr) return;
    static_cast<ST7789*>(handle)->DrawChineseString(x, y, str);
}

// ============== 图像操作 ==============

void ST7789_DrawImage(ST7789_Handle handle, uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t *pImage)
{
    if (handle == nullptr || pImage == nullptr) return;
    static_cast<ST7789*>(handle)->DrawImage(x, y, width, height, pImage);
}

void ST7789_CopyBuffer(ST7789_Handle handle, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t *DataBuff)
{
    if (handle == nullptr || DataBuff == nullptr) return;
    static_cast<ST7789*>(handle)->CopyBuffer(x, y, width, height, DataBuff);
}

// ============== USB MIDI 专用函数 ==============

void ST7789_ShowStatus(ST7789_Handle handle, const char* status)
{
    if (handle == nullptr || status == nullptr) return;
    
    ST7789* lcd = static_cast<ST7789*>(handle);
    lcd->SetColor(0xFFFFFF);  // 白色
    lcd->SetBackColor(0x000000);  // 黑色
    lcd->FillRect(0, 0, 320, 20);  // 顶部状态栏
    lcd->DrawString(5, 2, status);
}

void ST7789_ShowMidiMessage(ST7789_Handle handle, uint8_t channel, uint8_t note, uint8_t velocity)
{
    if (handle == nullptr) return;
    
    ST7789* lcd = static_cast<ST7789*>(handle);
    char buffer[64];
    
    // 设置新颜色
    lcd->SetColor(0x00FF00);  // 绿色
    lcd->SetBackColor(0x000000);
    
    snprintf(buffer, sizeof(buffer), "CH:%d Note:%d Vel:%d", channel, note, velocity);
    lcd->DrawString(10, 30, buffer);
}

void ST7789_ShowUsbStatus(ST7789_Handle handle, const char* usb_status)
{
    if (handle == nullptr || usb_status == nullptr) return;
    
    ST7789* lcd = static_cast<ST7789*>(handle);
    char buffer[32];
    
    // 根据状态设置颜色
    if (strstr(usb_status, "Connect") != NULL || strstr(usb_status, "Ready") != NULL) {
        lcd->SetColor(0x00FF00);  // 绿色表示正常
    } else if (strstr(usb_status, "Error") != NULL || strstr(usb_status, "Fail") != NULL) {
        lcd->SetColor(0xFF0000);  // 红色表示错误
    } else {
        lcd->SetColor(0xFFFF00);  // 黄色表示警告/等待
    }
    
    lcd->SetBackColor(0x000000);
    
    snprintf(buffer, sizeof(buffer), "USB: %s", usb_status);
    lcd->DrawString(10, 220, buffer);
}

} // extern "C"