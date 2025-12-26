# 运行于FKH750XBH6(32Bits SDRAM)的LUA Shell解释器

![Image](./Assets/ScreenShot.png)

## 原理
首先通过USART1接收命令行字节并匹配Terminal的行编辑模式并实时回显，然后在接收到方向键时移动光标或者翻历史记录(最高10条，要增减自己改MAX_HISTORY定义)

然后在接收到回车符号("\r"或"\n")后，将字符串指针通过队列送到LUA_ProcessTask_Handle，通过xQueueReceive把指针拷出来，此时指向的就是接收到的指令字符串数据，用完free

之后就是在接收到指令时，调用LUA解释器，执行指令

最后，就是错误处理，把出现的所有错误给接下来，语法和执行错误别把RTOS打崩了就行

## 如何对接新硬件？
首先，把你想要实现的功能写成函数，比如:

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

然后把函数的简名放在注册函数结构体数组，比如:

    const luaL_Reg hw_functions[] = {
        {"led",     lua_hw_led},
        {"btn",     lua_hw_button},
        {"delay",   lua_hw_delay},
        {"help",    lua_hw_help},
        // 可以继续添加 {"pwm", lua_pwm}, {"adc", lua_adc} 等
        {NULL, NULL} // 结束标记
    };

## 许可证

本项目采用**双许可证**模式：

- **个人/非商业使用**：遵循 [GPLv3 许可证](License.txt)
- **商业使用**：需获取商业授权，请联系 [xuelin-sherlyn@outlook.com](mailto:xuelin-sherlyn@outlook.com)

简单说：个人、学习、研究随便用；公司商用请先联系我获取授权。