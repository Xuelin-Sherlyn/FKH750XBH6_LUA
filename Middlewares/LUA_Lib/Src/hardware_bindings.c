// hardware_bindings.c - 集中管理所有硬件函数
#include "hardware_bindings.h"
#include "lua.h"
#include "lauxlib.h"
#include "FreeRTOS.h"
#include "portable.h"
#include "task.h"
#include "main.h" // 包含 HAL_GPIO_WritePin 等定义
#include "usart.h"

extern I2C_HandleTypeDef hi2c1;

// 1. 注册函数结构体数组（便于批量注册）
const luaL_Reg hw_functions[] = {
    {"led",         lua_hw_led},
    {"btn",         lua_hw_button},
    {"delay",       lua_hw_delay},
    {"i2c_send",    lua_hw_i2c_send},
    {"i2c_recv",    lua_hw_i2c_recv},
    {"i2c_writereg",lua_hw_i2c_write_reg},
    {"i2c_readreg", lua_hw_i2c_read_reg},
    {"help",        lua_hw_help},
    // 可以继续添加 {"pwm", lua_pwm}, {"adc", lua_adc} 等
    {NULL, NULL} // 结束标记
};

// 2. 库的打开函数（Lua调用 require "hw" 时会调用此函数）
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

// 3. 初始化函数（在你的main.c或嵌入式初始化中调用）
void hardware_bindings_init(lua_State* L) {
    // 注册硬件模块为全局表 "hardware"
    luaL_requiref(L, "hardware", luaopen_hardware, 1);
    lua_pop(L, 1); // 移除require留下的副本
}

// 4. 在头文件声明所有硬件函数并在源文件实现函数
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

int lua_hw_button(lua_State* L) {
    GPIO_PinState s = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
    lua_pushinteger(L, (s == GPIO_PIN_SET) ? 1 : 0);
    return 0;
}

int lua_hw_delay(lua_State* L) {
    int ms = luaL_checkinteger(L, 1);
    if(ms < 0 || ms > 60000) {
        return luaL_error(L, "delay ms out of range (0-60000)");
    }
    vTaskDelay(ms); // 注意：在RTOS任务中考虑使用 vTaskDelay
    return 0;
}

int lua_hw_i2c_send(lua_State *L)
{
    // 获取参数个数
    int argc = lua_gettop(L);
    if (argc < 2)
    {
        return luaL_error(L, "i2c.send requires address and data");
    }
    
    // 获取地址参数（7位地址）
    lua_Integer addr = luaL_checkinteger(L, 1);
    if (addr < 0 || addr > 0x7F)
    {
        return luaL_error(L, "invalid I2C address: 0x%02X", addr);
    }
    
    // 获取数据表
    luaL_checktype(L, 2, LUA_TTABLE);
    int data_len = luaL_len(L, 2);
    
    // 准备数据缓冲区
    uint8_t *data = (uint8_t *)lua_newuserdata(L, data_len);
    if (!data)
    {
        return luaL_error(L, "memory allocation failed");
    }
    
    // 从 Lua 表中读取数据
    for (int i = 0; i < data_len; i++)
    {
        lua_geti(L, 2, i + 1);  // Lua 索引从1开始
        lua_Integer byte = luaL_checkinteger(L, -1);
        if (byte < 0 || byte > 0xFF)
        {
            lua_pop(L, 1);  // 弹出无效值
            return luaL_error(L, "invalid byte value at index %d: %d", i+1, byte);
        }
        data[i] = (uint8_t)byte;
        lua_pop(L, 1);  // 弹出已读取的值
    }
    
    // 执行 I2C 传输
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(
        &hi2c1, 
        (uint16_t)(addr << 1),  // 左移1位（7位地址转8位）
        data, 
        data_len, 
        HAL_MAX_DELAY
    );
    
    // 释放临时数据
    lua_pop(L, 1);  // 弹出 userdata
    
    // 返回结果
    if (status == HAL_OK)
    {
        lua_pushboolean(L, 1);
        return 1;
    }
    else
    {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "I2C transmission failed");
        return 2;
    }
}

int lua_hw_i2c_recv(lua_State *L)
{
    lua_Integer addr = luaL_checkinteger(L, 1);
    lua_Integer length = luaL_checkinteger(L, 2);
    
    if (length <= 0 || length > 256)
    {
        return luaL_error(L, "invalid read length: %d", length);
    }
    
    // 分配接收缓冲区
    uint8_t *buffer = (uint8_t *)pvPortMalloc(length);
    if (!buffer)
    {
        return luaL_error(L, "memory allocation failed");
    }
    
    // 执行 I2C 接收
    HAL_StatusTypeDef status = HAL_I2C_Master_Receive(
        &hi2c1,
        (uint16_t)(addr << 1),
        buffer,
        length,
        HAL_MAX_DELAY
    );
    
    // 创建 Lua 表并填充数据
    if (status == HAL_OK)
    {
        lua_newtable(L);
        for (int i = 0; i < length; i++)
        {
            lua_pushinteger(L, buffer[i]);
            lua_seti(L, -2, i + 1);  // Lua 索引从1开始
        }
    }
    
    // 释放缓冲区
    vPortFree(buffer);
    
    if (status != HAL_OK)
    {
        lua_pushnil(L);
        lua_pushstring(L, "I2C reception failed");
        return 2;
    }
    
    return 1;  // 返回数据表
}

int lua_hw_i2c_write_reg(lua_State *L)
{
    lua_Integer addr = luaL_checkinteger(L, 1);
    lua_Integer reg = luaL_checkinteger(L, 2);
    
    // 准备数据缓冲区
    uint8_t *buffer = NULL;
    int buffer_len = 0;
    
    // 第三个参数可以是整数（单字节）或表（多字节）
    if (lua_isinteger(L, 3))
    {
        // 单字节写入
        buffer_len = 1;
        buffer = (uint8_t *)pvPortMalloc(buffer_len + 1); // +1 用于寄存器地址
        if (!buffer)
        {
            return luaL_error(L, "memory allocation failed");
        }
        buffer[0] = (uint8_t)reg; // 寄存器地址
        buffer[1] = (uint8_t)lua_tointeger(L, 3); // 数据
    }
    else if (lua_istable(L, 3))
    {
        // 多字节写入
        buffer_len = luaL_len(L, 3);
        buffer = (uint8_t *)pvPortMalloc(buffer_len + 1); // +1 用于寄存器地址
        if (!buffer)
        {
            return luaL_error(L, "memory allocation failed");
        }
        buffer[0] = (uint8_t)reg; // 寄存器地址
        
        // 从表中读取数据
        for (int i = 0; i < buffer_len; i++)
        {
            lua_geti(L, 3, i + 1);
            lua_Integer byte = luaL_checkinteger(L, -1);
            if (byte < 0 || byte > 0xFF)
            {
                vPortFree(buffer);
                lua_pop(L, 1);
                return luaL_error(L, "invalid byte value at index %d: %d", i+1, byte);
            }
            buffer[i + 1] = (uint8_t)byte;
            lua_pop(L, 1);
        }
    }
    else
    {
        return luaL_error(L, "expected integer or table for data");
    }
    
    // 执行I2C传输，发送寄存器地址和数据
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(
        &hi2c1,
        (uint16_t)(addr << 1),
        buffer,
        buffer_len + 1, // 包括寄存器地址
        HAL_MAX_DELAY
    );
    
    vPortFree(buffer);
    
    if (status == HAL_OK)
    {
        lua_pushboolean(L, 1);
        return 1;
    }
    else
    {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "I2C write register failed");
        return 2;
    }
}

int lua_hw_i2c_read_reg(lua_State *L)
{
    lua_Integer addr = luaL_checkinteger(L, 1);
    lua_Integer reg = luaL_checkinteger(L, 2);
    lua_Integer length = luaL_optinteger(L, 3, 1);
    
    // 发送寄存器地址
    uint8_t reg_addr = (uint8_t)reg;
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(
        &hi2c1,
        (uint16_t)(addr << 1),
        &reg_addr,
        1,
        HAL_MAX_DELAY
    );
    
    if (status != HAL_OK)
    {
        lua_pushnil(L);
        lua_pushstring(L, "failed to send register address");
        return 2;
    }
    
    // 接收数据
    uint8_t *buffer = (uint8_t *)pvPortMalloc(length);
    status = HAL_I2C_Master_Receive(
        &hi2c1,
        (uint16_t)(addr << 1) | 0x01,
        buffer,
        length,
        HAL_MAX_DELAY
    );
    
    if (status == HAL_OK)
    {
        if (length == 1)
        {
            // 单个字节直接返回值
            lua_pushinteger(L, buffer[0]);
        }
        else
        {
            // 多个字节返回表
            lua_newtable(L);
            for (int i = 0; i < length; i++)
            {
                lua_pushinteger(L, buffer[i]);
                lua_seti(L, -2, i + 1);
            }
        }
    }
    
    vPortFree(buffer);
    
    if (status != HAL_OK)
    {
        lua_pushnil(L);
        lua_pushstring(L, "failed to read register data");
        return 2;
    }
    
    return 1;
}

int lua_hw_help(lua_State* L) {
    safe_printf("\r\n\033[36m=== Hardware API Help ===\033[0m\r\n");
    safe_printf("hardware.led(state)                            - Control LED (0=OFF, 1=ON)\r\n");
    safe_printf("hardware.button                                - Read Button (0=Release, 1=Press)\r\n");
    safe_printf("hardware.delay(ms)                             - Delay in milliseconds\r\n");
    safe_printf("hardware.i2c_send(address, {data})             - Write I2C Devices\r\n");
    safe_printf("hardware.i2c_recv(address, length)             - Read I2C Devices\r\n");
    safe_printf("hardware.i2c_writereg(address, reg, length)    - Write I2C Devices Reg\r\n");
    safe_printf("hardware.i2c_readreg(address, reg, length)     - read I2C Devices Reg\r\n");
    // 更多帮助信息...
    safe_printf("\r\nType 'hardware' to see available functions\r\n");
    return 0;
}