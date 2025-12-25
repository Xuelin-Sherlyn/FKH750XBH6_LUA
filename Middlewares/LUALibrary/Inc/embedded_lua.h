// Middlewares/LUALibrary/Inc/embedded_lua.h
#ifndef EMBEDDED_LUA_H
#define EMBEDDED_LUA_H

#include "lua.h"       // 来自 Src/lua
#include "lualib.h"
#include "lauxlib.h"

// 简化初始化：创建Lua状态机并注册基础硬件函数
lua_State* embedded_lua_init(void);

// 执行字符串脚本，返回Lua状态码 (LUA_OK 为成功)
int embedded_lua_execute(lua_State* L, const char* script);

// 注册额外的C函数到Lua全局环境
void embedded_lua_register_function(lua_State* L, const char* name, lua_CFunction func);

// 清理函数
void embedded_lua_close(lua_State* L);

#endif