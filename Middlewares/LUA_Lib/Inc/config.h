// config.h
#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

// Lua 配置文件结构
typedef struct {
    bool auto_start_lua;       // 是否自动启动 Lua 脚本
    bool enable_sandbox;       // 是否启用沙箱模式
    int max_file_size;         // 最大文件大小（字节）
    int max_recursion_depth;   // 最大递归深度
    bool enable_debug;         // 是否启用调试输出
    char startup_script[128];  // 启动脚本路径
    char lua_path[256];        // Lua 模块搜索路径
} LuaConfig;

// 默认配置
#define DEFAULT_CONFIG { \
    .auto_start_lua = true, \
    .enable_sandbox = false, \
    .max_file_size = 1024 * 1024,  /* 1MB */ \
    .max_recursion_depth = 10, \
    .enable_debug = false, \
    .startup_script = "0:/startup.lua", \
    .lua_path = "0:/lua/?.lua;0:/scripts/?.lua;?.lua" \
}

// 配置文件读取/保存
bool config_load(LuaConfig *config, const char *filename);
bool config_save(const LuaConfig *config, const char *filename);
void config_print(const LuaConfig *config);

#endif // CONFIG_H