#include "lua_display.h"
#include "lua.h"
#include "lauxlib.h"
#include "main.h"
#include "spi.h"
#include "st7789_c_interface.h"
#include <stdint.h>
#include <sys/_intsup.h>

ST7789_Handle g_lcd_handle = NULL;

// 1. 注册函数结构体数组（便于批量注册）
const luaL_Reg display_functions[] = {
    {"BackLight", lua_display_backlight},
    {"Init", lua_display_init},
    {"WriteLine", lua_display_writeline},
    {"Clear", lua_display_clear}
};

// 2. 库的打开函数（Lua调用 require "display" 时会调用此函数）
int luaopen_display(lua_State* L)
{
    luaL_newlib(L, display_functions);
    return 1;
}

// 3. 初始化函数（在你的main.c或嵌入式初始化中调用）
void display_bindings_init(lua_State* L) {
    // 注册屏幕模块为全局表 "display"
    luaL_requiref(L, "display", luaopen_display, 1);
    lua_pop(L, 1); // 移除require留下的副本
}

int lua_display_backlight(lua_State* L)
{
    if(!lua_isnumber(L, 1))
    {
        lua_pushstring(L, "display.Backlight: argument #1 must be a number (0 or 1)");
        return lua_error(L); // 这会安全地触发Lua错误，而不是崩溃
    }
    int state = lua_tointeger(L, 1);
    if (state != 0 && state != 1) {
        lua_pushstring(L, "display.Backlight: state must be 0 or 1");
        return lua_error(L);
    }
    switch (state) {
        case 1:
            LCD_Backlight_ON;
            break;
        case 0:
            LCD_Backlight_OFF;
            break;
        default:
            break;
    }
    return 0;
}

int lua_display_init(lua_State* L)
{
    g_lcd_handle = ST7789_Create(&hspi6);
    if (g_lcd_handle) {
        ST7789_InitDisplay(g_lcd_handle);
        ST7789_SetFont(g_lcd_handle, &ASCII_10x20);
        ST7789_SetColor(g_lcd_handle, 0xFF00FFFF);
        ST7789_SetBackColor(g_lcd_handle, 0xFF000000);
        ST7789_Clear(g_lcd_handle);
        LCD_Backlight_ON;
        return 0;
    }
    lua_pushstring(L, "Display Init Fail");
    return lua_error(L); // 这会安全地触发Lua错误，而不是崩溃
}

int lua_display_writeline(lua_State* L)
{
    if(!lua_isnumber(L, 1) && !lua_isnumber(L, 2) && !lua_isstring(L, 3))
    {
        lua_pushstring(L, "display.DrawLine: argument must be (int)X, (int)Y, (LPCWSTR)String");
        return lua_error(L); // 这会安全地触发Lua错误，而不是崩溃
    }
    int X = lua_tointeger(L, 1);
    int Y = lua_tointeger(L, 2);
    const char* str = lua_tostring(L, 3);
    uint8_t FontColor = lua_tointeger(L, 4);
    uint8_t BackColor = lua_tointeger(L, 5);
    if (X < 0 && Y < 1 && str == NULL) {
        lua_pushstring(L, "display.Drawline: Arguments Error");
        return lua_error(L);
    }
    if(FontColor)
    {
        switch(FontColor)
        {
            case 30:
                ST7789_SetColor(g_lcd_handle, 0xFF000000);
                break;
            case 31:
                ST7789_SetColor(g_lcd_handle, 0xFFFF0000);
                break;
            case 32:
                ST7789_SetColor(g_lcd_handle, 0xFF00FF00);
                break;
            case 33:
                ST7789_SetColor(g_lcd_handle, 0xFFFFFF00);
                break;
            case 34:
                ST7789_SetColor(g_lcd_handle, 0xFF0000FF);
                break;
            case 35:
                ST7789_SetColor(g_lcd_handle, 0xFFFF00FF);
                break;
            case 36:
                ST7789_SetColor(g_lcd_handle, 0XFF00FFFF);
                break;
            case 37:
                ST7789_SetColor(g_lcd_handle, 0xFFFFFFF);
                break;
            default:
                break;
        }
    }
    else {
        ST7789_SetColor(g_lcd_handle, 0xFFFFFFFF);
    }
    if(BackColor)
    {
        switch(BackColor)
        {
            case 40:
                ST7789_SetBackColor(g_lcd_handle, 0xFF000000);
                break;
            case 41:
                ST7789_SetBackColor(g_lcd_handle, 0xFFFF0000);
                break;
            case 42:
                ST7789_SetBackColor(g_lcd_handle, 0xFF00FF00);
                break;
            case 43:
                ST7789_SetBackColor(g_lcd_handle, 0xFFFFFF00);
                break;
            case 44:
                ST7789_SetBackColor(g_lcd_handle, 0xFF0000FF);
                break;
            case 45:
                ST7789_SetBackColor(g_lcd_handle, 0xFFFF00FF);
                break;
            case 46:
                ST7789_SetBackColor(g_lcd_handle, 0XFF00FFFF);
                break;
            case 47:
                ST7789_SetBackColor(g_lcd_handle, 0xFFFFFFF);
                break;
            default:
                break;
        }
    }
    else {
        ST7789_SetBackColor(g_lcd_handle, 0xFF000000);
    }
    ST7789_DrawString(g_lcd_handle, X, Y, str);
    return 0;
}

int lua_display_clear(lua_State* L)
{
    ST7789_SetBackColor(g_lcd_handle, 0xFF000000);
    ST7789_Clear(g_lcd_handle);
    return 0;
}