// lua_fatfs.h
#ifndef LUA_FATFS_H
#define LUA_FATFS_H

#include "lua.h"
#include "lauxlib.h"
#include "ff.h"
#include "diskio.h"

/**
 * @brief 初始化FatFS绑定库，并注册到指定的Lua状态机。
 * 
 * 这是最主要的函数，在你的应用启动Lua后调用一次。
 * 它会在Lua全局环境中创建名为 `fatfs` 的表，所有FatFS函数都注册在其中。
 * 
 * @param L 已初始化的Lua状态机指针。
 */
void fatfs_bindings_init(lua_State* L);

/**
 * @brief FatFS模块的Lua “包加载器” 函数。
 * 
 * 遵循Lua C模块规范。当Lua脚本中执行 `require “fatfs”` 时会调用此函数。
 * 通常你不需要直接调用它，`fatfs_bindings_init` 内部会使用。
 * 
 * @param L Lua状态机指针。
 * @return 返回包含所有硬件函数的Lua表（即模块）。
 */
 int luaopen_fatfs(lua_State *L);

/**
 * @brief 挂载驱动器。
 * Lua用法: `fatfs.mount()`
 */
int lua_fatfs_mount(lua_State *L);

/**
 * @brief 卸载驱动器。
 * Lua用法: `fatfs.umount()`
 */
int lua_fatfs_umount(lua_State *L);

/**
 * @brief 列出目录。
 * Lua用法: `fatfs.ls(<(string)Path>)`
 */
int lua_fatfs_ls(lua_State *L);

/**
 * @brief 删除文件。
 * Lua用法: `fatfs.rm(<(string)File Path>)`
 */
int lua_fatfs_rm(lua_State *L);

/**
 * @brief 创建目录。
 * Lua用法: `fatfs.mkdir(<(string)Folder Path>)`
 */
int lua_fatfs_mkdir(lua_State *L);

/**
 * @brief 删除文件夹。
 * Lua用法: `fatfs.rmdir(<(string)Folder Path>)`
 */
int lua_fatfs_rmdir(lua_State *L);

/**
 * @brief 获取空余大小。
 * Lua用法: `fatfs.getfree(<(string)Path>)`
 */
int lua_fatfs_getfree(lua_State *L);

/**
 * @brief 帮助函数。
 * Lua用法: `fatfs.help`
 */
int lua_fatfs_help(lua_State* L);

#endif // LUA_FATFS_H