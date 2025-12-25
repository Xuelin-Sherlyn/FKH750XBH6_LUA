// hardware_bindings.h
#ifndef HARDWARE_BINDINGS_H
#define HARDWARE_BINDINGS_H

#ifdef __cplusplus
extern "C" {
#endif

// 包含Lua核心头文件，确保所有函数签名正确
#include "lua.h"
#include "lauxlib.h"

/**
 * @brief 初始化硬件绑定库，并注册到指定的Lua状态机。
 * 
 * 这是最主要的函数，在你的应用启动Lua后调用一次。
 * 它会在Lua全局环境中创建名为 `hw` 的表，所有硬件函数都注册在其中。
 * 
 * @param L 已初始化的Lua状态机指针。
 */
void hardware_bindings_init(lua_State* L);

/**
 * @brief 硬件模块的Lua “包加载器” 函数。
 * 
 * 遵循Lua C模块规范。当Lua脚本中执行 `require “hardware”` 时会调用此函数。
 * 通常你不需要直接调用它，`hardware_bindings_init` 内部会使用。
 * 
 * @param L Lua状态机指针。
 * @return 返回包含所有硬件函数的Lua表（即模块）。
 */
int luaopen_hardware(lua_State* L);

/* 
 * 以下是为特定硬件操作编写的C函数声明。
 * 它们被注册到Lua的 `hw` 表中，供脚本调用。
 * 每个函数都对应一个具体的硬件操作。
 */

/**
 * @brief 控制LED。
 * Lua用法: `hw.led(1)` 或 `hw.led(0)`
 */
int lua_led(lua_State* L);

/**
 * @brief 读取按键状态。
 * Lua用法: `state = hw.btn()`
 */
int lua_button(lua_State* L);

/**
 * @brief 毫秒级延时（阻塞）。
 * Lua用法: `hw.delay(100)`
 * 注意：在RTOS中，你可能想用非阻塞的 `vTaskDelay` 重写此函数。
 */
int lua_delay(lua_State* L);

/**
 * @brief 设置PWM输出（示例，需要根据你的定时器配置修改）。
 * Lua用法: `hw.pwm(channel, frequency, duty_cycle)`
 */
// static int lua_pwm(lua_State* L); // 按需启用

/**
 * @brief 读取ADC值（示例）。
 * Lua用法: `value = hw.adc(channel)`
 */
// static int lua_adc(lua_State* L); // 按需启用

#ifdef __cplusplus
}
#endif

#endif // HARDWARE_BINDINGS_H