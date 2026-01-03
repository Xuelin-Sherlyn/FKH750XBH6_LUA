// lua_file_execute.c
#include "lua_file_execute.h"
#include "ff.h"
#include "usart.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// 全局跳转缓冲区
jmp_buf g_lua_panic_jmp;

// 全局配置
static LuaConfig g_lua_config;

// Lua流式读取器结构
typedef struct {
    FIL *file;
    char buffer[512];
    size_t buffer_pos;
    size_t buffer_size;
} LuaStreamReader;

// Lua紧急错误回调
int lua_panic_handler(lua_State* L) {
    const char* msg = lua_tostring(L, -1);
    if (msg == NULL) msg = "unknown panic";
    safe_printf("\r\n\033[31m[LUA PANIC] %s\033[0m\r\n", msg);
    
    // 跳转到安全恢复点
    longjmp(g_lua_panic_jmp, 1);
    return 0;
}

// 安全的Lua初始化
lua_State* safe_lua_init(void) {
    lua_State* L = luaL_newstate();
    if (!L) return NULL;
    
    // 设置紧急处理器
    lua_atpanic(L, lua_panic_handler);
    
    if (setjmp(g_lua_panic_jmp) == 0) {
        // 正常初始化流程
        luaL_openlibs(L);
        return L;
    } else {
        // 从Panic中恢复
        safe_printf("\033[33mRecreating Lua VM after panic...\033[0m\r\n");
        lua_close(L);
        return safe_lua_init();
    }
}

// 安全的Lua代码执行
int safe_lua_execute(lua_State* L, const char* code, const char* cmd_name) {
    // 1. 加载代码（捕获语法错误）
    int load_status = luaL_loadstring(L, code);
    if (load_status != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        safe_printf("\r\n\033[31mSyntax Error: %s\033[0m\r\n", err);
        lua_pop(L, 1);
        return -1;
    }
    
    // 2. 在保护模式下执行
    int exec_status = lua_pcall(L, 0, LUA_MULTRET, 0);
    
    // 3. 处理执行结果
    if (exec_status == LUA_OK) {
        // 成功 - 处理返回值
        int nresults = lua_gettop(L);
        if (nresults > 0) {
            safe_printf("\r\n\033[32mResult(s):\033[0m ");
            for (int i = 1; i <= nresults; i++) {
                if (lua_isinteger(L, i)) {
                    safe_printf("%lld ", lua_tointeger(L, i));
                } else if (lua_isstring(L, i)) {
                    safe_printf("%s ", lua_tostring(L, i));
                } else if (lua_isboolean(L, i)) {
                    safe_printf(lua_toboolean(L, i) ? "true " : "false ");
                }
            }
            safe_printf("\r\n");
            lua_pop(L, nresults);
        }
        safe_printf("\r\n\033[32m%s: OK\033[0m\r\n", cmd_name);
        return 0;
    }
    else {
        // 运行时错误
        const char* err = lua_tostring(L, -1);
        const char* err_type = 
            (exec_status == LUA_ERRRUN) ? "Runtime Error" :
            (exec_status == LUA_ERRMEM) ? "Memory Error" :
            (exec_status == LUA_ERRERR) ? "Error Handler Error" : "Unknown Error";
        
        safe_printf("\r\n\033[31m%s: %s\033[0m\r\n", err_type, err);
        lua_pop(L, 1);
        return -2;
    }
}

// 流式读取器函数
static const char* lua_stream_reader(lua_State* L, void* data, size_t* size) {
    LuaStreamReader* reader = (LuaStreamReader*)data;
    
    // 如果缓冲区已空，从文件读取更多数据
    if (reader->buffer_pos >= reader->buffer_size) {
        UINT bytes_read;
        FRESULT fr = f_read(reader->file, reader->buffer, sizeof(reader->buffer), &bytes_read);
        
        if (fr != FR_OK || bytes_read == 0) {
            *size = 0;
            return NULL;  // 文件结束或错误
        }
        
        reader->buffer_pos = 0;
        reader->buffer_size = bytes_read;
    }
    
    // 返回缓冲区中剩余的数据
    *size = reader->buffer_size - reader->buffer_pos;
    const char* chunk = reader->buffer + reader->buffer_pos;
    reader->buffer_pos = reader->buffer_size;
    
    return chunk;
}

// Lua文件执行函数（一次性加载）
int lua_file_execute(lua_State* L, const char* filename) {
    FIL file;
    FRESULT fr;
    UINT bytes_read;
    
    // 打开文件
    fr = f_open(&file, filename, FA_READ);
    if (fr != FR_OK) {
        safe_printf("\r\033[31mCan`t Open File: %s (Error: %d)\033[0m\r\n", filename, fr);
        return -1;
    }
    
    // 获取文件大小
    FSIZE_t file_size = f_size(&file);
    
    // 检查文件大小是否超过配置限制
    if (file_size > g_lua_config.max_file_size) {
        safe_printf("\r\033[31m File to Big: %s (%lu Byte > %d Byte)\033[0m\r\n", 
                   filename, (unsigned long)file_size, 
                   g_lua_config.max_file_size);
        f_close(&file);
        return -2;
    }
    
    // 分配内存（额外一个字节用于字符串终止符）
    char* buffer = (char*)pvPortMalloc(file_size + 1);
    if (!buffer) {
        safe_printf("\r\033[31mMemory Malloc Error\033[0m\r\n");
        f_close(&file);
        return -3;
    }
    
    // 读取文件
    fr = f_read(&file, buffer, file_size, &bytes_read);
    f_close(&file);
    
    if (fr != FR_OK || bytes_read != file_size) {
        safe_printf("\r\033[31mRead File Fail: Expectation %lu Byte, Read %u Byte\033[0m\r\n", 
                   (unsigned long)file_size, bytes_read);
        vPortFree(buffer);
        return -4;
    }
    
    buffer[file_size] = '\0';
    
    // 执行Lua代码
    int result = safe_lua_execute(L, buffer, filename);
    
    vPortFree(buffer);
    return result;
}

// 流式执行Lua文件（避免大内存分配）
int lua_file_execute_stream(lua_State* L, const char* filename) {
    FIL file;
    FRESULT fr;
    
    // 打开文件
    fr = f_open(&file, filename, FA_READ);
    if (fr != FR_OK) {
        safe_printf("\r\033[31m无法打开文件: %s (错误: %d)\033[0m\r\n", filename, fr);
        return -1;
    }
    
    // 检查文件大小
    FSIZE_t file_size = f_size(&file);
    if (file_size > g_lua_config.max_file_size) {
        safe_printf("\r\033[31m文件过大: %s (%lu 字节 > %d 字节)\033[0m\r\n", 
                   filename, (unsigned long)file_size, 
                   g_lua_config.max_file_size);
        f_close(&file);
        return -2;
    }
    
    // 准备流式读取器
    LuaStreamReader reader = {
        .file = &file,
        .buffer_pos = 0,
        .buffer_size = 0
    };
    
    // 加载Lua代码（流式）
    int load_result = lua_load(L, lua_stream_reader, &reader, filename, NULL);
    
    // 关闭文件
    f_close(&file);
    
    if (load_result != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        safe_printf("\033[31mLua Load Error: %s\033[0m\r\n", err);
        lua_pop(L, 1);
        return -3;
    }
    
    // 执行加载的代码
    int exec_result = lua_pcall(L, 0, LUA_MULTRET, 0);
    
    if (exec_result != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        safe_printf("\r\033[31mLua Execute Error: %s\033[0m\r\n", err);
        lua_pop(L, 1);
        return -4;
    }
    
    // 处理返回值
    int nresults = lua_gettop(L);
    if (nresults > 0) {
        safe_printf("\r\n\033[32mResult(s):\033[0m ");
        for (int i = 1; i <= nresults; i++) {
            if (lua_isinteger(L, i)) {
                safe_printf("%lld ", lua_tointeger(L, i));
            } else if (lua_isstring(L, i)) {
                safe_printf("%s ", lua_tostring(L, i));
            } else if (lua_isboolean(L, i)) {
                safe_printf(lua_toboolean(L, i) ? "true " : "false ");
            }
        }
        safe_printf("\r\n");
        lua_pop(L, nresults);
    }
    
    safe_printf("\r\n\033[32m%s: OK\033[0m\r\n", filename);
    return 0;
}

// 检查文件是否存在
bool lua_file_exists(const char* filename) {
    FILINFO fno;
    return f_stat(filename, &fno) == FR_OK;
}

// 获取文件大小
long lua_file_size(const char* filename) {
    FILINFO fno;
    if (f_stat(filename, &fno) == FR_OK) {
        return (long)fno.fsize;
    }
    return -1;
}

// 从Lua访问配置信息
int lua_get_config(lua_State* L) {
    lua_newtable(L);
    
    lua_pushstring(L, "auto_start_lua");
    lua_pushboolean(L, g_lua_config.auto_start_lua);
    lua_settable(L, -3);
    
    lua_pushstring(L, "enable_sandbox");
    lua_pushboolean(L, g_lua_config.enable_sandbox);
    lua_settable(L, -3);
    
    lua_pushstring(L, "max_file_size");
    lua_pushinteger(L, g_lua_config.max_file_size);
    lua_settable(L, -3);
    
    lua_pushstring(L, "max_recursion_depth");
    lua_pushinteger(L, g_lua_config.max_recursion_depth);
    lua_settable(L, -3);
    
    lua_pushstring(L, "enable_debug");
    lua_pushboolean(L, g_lua_config.enable_debug);
    lua_settable(L, -3);
    
    lua_pushstring(L, "startup_script");
    lua_pushstring(L, g_lua_config.startup_script);
    lua_settable(L, -3);
    
    lua_pushstring(L, "lua_path");
    lua_pushstring(L, g_lua_config.lua_path);
    lua_settable(L, -3);
    
    return 1;
}

// 创建默认启动脚本
static void create_default_startup_script(const char* filename) {
    FIL file;
    FRESULT fr = f_open(&file, filename, FA_WRITE | FA_CREATE_ALWAYS);
    
    if (fr == FR_OK) {
        UINT bytes_written;
        const char* default_script = 
            "-- 默认启动脚本\n"
            "print(\"=== STM32H750 Lua 系统 ===\")\n"
            "print(\"启动时间: \" .. os.time())\n"
            "\n"
            "-- 检查系统配置\n"
            "if config then\n"
            "    print(\"配置信息:\")\n"
            "    print(\"  自动启动: \" .. (config.auto_start_lua and \"启用\" or \"禁用\"))\n"
            "    print(\"  沙箱模式: \" .. (config.enable_sandbox and \"启用\" or \"禁用\"))\n"
            "    print(string.format(\"  最大文件大小: %.2f MB\", config.max_file_size / 1024 / 1024))\n"
            "end\n"
            "\n"
            "-- 检查模块\n"
            "if fatfs then\n"
            "    print(\"FatFS模块: 可用\")\n"
            "    local disk = fatfs.getfree()\n"
            "    if disk then\n"
            "        print(string.format(\"磁盘空间: %.2f MB 可用\", \n"
            "            disk.free_bytes / 1024 / 1024))\n"
            "    end\n"
            "else\n"
            "    print(\"FatFS模块: 不可用\")\n"
            "end\n"
            "\n"
            "if hardware then\n"
            "    print(\"硬件模块: 可用\")\n"
            "else\n"
            "    print(\"硬件模块: 不可用\")\n"
            "end\n"
            "\n"
            "-- 设置Lua路径（如果支持）\n"
            "if package and config then\n"
            "    package.path = config.lua_path\n"
            "    print(\"Lua路径已设置\")\n"
            "end\n"
            "\n"
            "print(\"=== 启动完成 ===\")\n";
        
        f_write(&file, default_script, strlen(default_script), &bytes_written);
        f_close(&file);
        safe_printf("\r\033[32mDefault startup script created success: %s\033[0m\r\n", filename);
    } else {
        safe_printf("\r\033[31mDefault startup script created Fail: %s\033[0m\r\n", filename);
    }
}

// 使用配置文件自动执行启动脚本
void lua_auto_execute_startup(lua_State* L) {
    safe_printf("\r\n\033[36m=== Check Auto Startup ===\033[0m\r\n");
    
    // 加载配置文件
    if (!config_load(&g_lua_config, "0:/lua.cfg")) {
        // safe_printf("\r\033[33mUsing default config\033[0m\r\n");
        
        // 保存默认配置到文件
        config_save(&g_lua_config, "0:/lua.cfg");
        safe_printf("\r\033[32mDefault config saveas 0:/lua.cfg\033[0m\r\n");
    }
    
    // 打印配置信息
    if (g_lua_config.enable_debug) {
        config_print(&g_lua_config);
    }
    
    // 检查是否启用自动启动
    if (!g_lua_config.auto_start_lua) {
        safe_printf("\033[33mAuto Startup Disable(Config auto_start_lua = false)\033[0m\r\n");
        return;
    }
    
    safe_printf("Startup Script: %s\r\n", g_lua_config.startup_script);
    
    // 检查启动脚本是否存在
    if (!lua_file_exists(g_lua_config.startup_script)) {
        safe_printf("\033[33mStartup script does not exist, creating default script...\033[0m\r\n");
        create_default_startup_script(g_lua_config.startup_script);
    }
    
    // 设置Lua路径
    lua_getglobal(L, "package");
    if (!lua_isnil(L, -1)) {
        lua_pushstring(L, "path");
        lua_pushstring(L, g_lua_config.lua_path);
        lua_settable(L, -3);
    }
    lua_pop(L, 1);  // 弹出 package
    
    // 将配置信息注册到Lua
    lua_pushcfunction(L, lua_get_config);
    lua_setglobal(L, "get_config");
    
    // 执行启动脚本
    safe_printf("\033[36mExecute Startup Script...\033[0m\r\n");
    
    int result;
    if (g_lua_config.enable_sandbox) {
        // 沙箱模式（这里可以添加沙箱限制）
        safe_printf("\033[33mSandBox Mode Execution(Some functionality may be limited)\033[0m\r\n");
        result = lua_file_execute_stream(L, g_lua_config.startup_script);
    } else {
        // 正常模式
        result = lua_file_execute_stream(L, g_lua_config.startup_script);
    }
    
    if (result == 0) {
        safe_printf("\033[32mStartup script execute sucess\033[0m\r\n");
    } else {
        safe_printf("\033[31mStartupn script execute fail (Error code: %d)\033[0m\r\n", result);
    }
    
    safe_printf("\033[36m=== Auto Startup end ===\033[0m\r\n");
}