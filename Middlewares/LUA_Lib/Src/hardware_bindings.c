// hardware_bindings.c - 集中管理所有硬件函数
#include "hardware_bindings.h"
#include "lua.h"
#include "lauxlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "main.h" // 包含 HAL_GPIO_WritePin 等定义
#include "usart.h"

// 1. 声明所有硬件函数（使用static限制作用域）
int lua_hw_led(lua_State* L) {
    // 使用 lua_isnumber 而不是直接转换
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "hw.led: argument #1 must be a number (0 or 1)");
        return lua_error(L); // 这会安全地触发Lua错误，而不是崩溃
    }
    
    int state = lua_tointeger(L, 1);
    if (state != 0 && state != 1) {
        lua_pushstring(L, "hw.led: state must be 0 or 1");
        return lua_error(L);
    }
    
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, (state) ? GPIO_PIN_RESET : GPIO_PIN_SET);
    return 0;
}

int lua_button(lua_State* L) {
    GPIO_PinState s = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
    lua_pushinteger(L, (s == GPIO_PIN_SET) ? 1 : 0);
    return 0;
}

int lua_delay(lua_State* L) {
    int ms = luaL_checkinteger(L, 1);
    if(ms < 0 || ms > 60000) {
        return luaL_error(L, "delay ms out of range (0-60000)");
    }
    vTaskDelay(ms); // 注意：在RTOS任务中考虑使用 vTaskDelay
    return 0;
}

int lua_hw_help(lua_State* L) {
    safe_printf("\r\n\033[36m=== Hardware API Help ===\033[0m\r\n");
    safe_printf("hardware.led(state)      - Control LED (0=OFF, 1=ON)\r\n");
    safe_printf("hardware.button          - Read Button (0=Release, 1=Press)\r\n");
    safe_printf("hardware.delay(ms)       - Delay in milliseconds\r\n");
    // 更多帮助信息...
    safe_printf("\r\nType 'hardware' to see available functions\r\n");
    return 0;
}

// 2. 注册函数结构体数组（便于批量注册）
const luaL_Reg hw_functions[] = {
    {"led",     lua_hw_led},
    {"btn",     lua_button},
    {"delay",   lua_delay},
    {"help",    lua_hw_help},
    // 可以继续添加 {"pwm", lua_pwm}, {"adc", lua_adc} 等
    {NULL, NULL} // 结束标记
};

// 3. 库的打开函数（Lua调用 require "hw" 时会调用此函数）
int luaopen_hardware(lua_State* L) {
    // 创建一个新的Lua表（库）
    luaL_newlib(L, hw_functions);
    
    // 可以添加一些常量到表中，例如引脚定义
    lua_pushinteger(L, GPIO_PIN_13);
    lua_setfield(L, -2, "PIN_LED");
    
    // lua_pushinteger(L, GPIO_PIN_0);
    // lua_setfield(L, -2, "PIN_BTN");
    
    // 返回这个表，Lua会将其作为模块
    return 1;
}

// 4. 初始化函数（在你的main.c或嵌入式初始化中调用）
void hardware_bindings_init(lua_State* L) {
    // 注册硬件模块为全局表 "hardware"
    luaL_requiref(L, "hardware", luaopen_hardware, 1);
    lua_pop(L, 1); // 移除require留下的副本
}