// lua_file_execute.h
#ifndef LUA_FILE_EXECUTE_H
#define LUA_FILE_EXECUTE_H

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "config.h"  // 添加 config.h 包含
#include <stdbool.h>
#include <setjmp.h>

// 全局跳转缓冲区（用于panic恢复）
extern jmp_buf g_lua_panic_jmp;

// Lua紧急错误回调
int lua_panic_handler(lua_State* L);

// 安全的Lua初始化
lua_State* safe_lua_init(void);

// 安全的Lua代码执行
int safe_lua_execute(lua_State* L, const char* code, const char* cmd_name);

// Lua文件执行函数（一次性加载）
int lua_file_execute(lua_State* L, const char* filename);

// 流式执行Lua文件（避免大内存分配）
int lua_file_execute_stream(lua_State* L, const char* filename);

// 使用配置文件自动执行启动脚本
void lua_auto_execute_startup(lua_State* L);

// 检查文件是否存在
bool lua_file_exists(const char* filename);

// 获取文件大小
long lua_file_size(const char* filename);

// 从Lua访问配置信息
int lua_get_config(lua_State* L);

#endif // LUA_FILE_EXECUTE_H