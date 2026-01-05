#include "main.h"
#include "lua.h"
#include "lauxlib.h"

#define COLOR_RED = 0xFFFF0000
#define COLOR_GREEN = 0XFF00FF00
#define COLOR_BLUE = 0XFF0000FF
#define COLOR_YELOW = 0XFFFFFF00
#define COLOR_ORANGE = 0XFFFFA500
#define COLOR_CYAN = 0XFF00FFFF
#define COLOR_PINK = 0xFFFF00FF
#define COLOR_WHITE = 0xFFFFFFFF


/**
 * @brief 初始化Display绑定库，并注册到指定的Lua状态机。
 * 
 * 这是最主要的函数，在你的应用启动Lua后调用一次。
 * 它会在Lua全局环境中创建名为 `display` 的表，所有FatFS函数都注册在其中。
 * 
 * @param L 已初始化的Lua状态机指针。
 */
int luaopen_display(lua_State* L);

/**
 * @brief Display模块的Lua “包加载器” 函数。
 * 
 * 遵循Lua C模块规范。当Lua脚本中执行 `require “display”` 时会调用此函数。
 * 通常你不需要直接调用它，`display_bindings_init` 内部会使用。
 * 
 * @param L Lua状态机指针。
 * @return 返回包含所有硬件函数的Lua表（即模块）。
 */
void display_bindings_init(lua_State* L);

/**
 * @brief 控制背光。
 * Lua用法: `display.Backlight(1)` 或 `display.Backlight(0)`
 */
int lua_display_backlight(lua_State* L);

/**
 * @brief 初始化显示器。
 * Lua用法: `display.Init()`
 */
int lua_display_init(lua_State* L);

/**
 * @brief 在指定坐标绘制一行字符串。
 * Lua用法: `display.WriteLine(<(int)X>,<int>Y,<(string)String>,<(int)>Color)`
 */
int lua_display_writeline(lua_State* L);

/**
 * @brief 清空显示器。
 * Lua用法: `display.Clear()`
 */
int lua_display_clear(lua_State* L);