// Middlewares/LUALibrary/Src/embedded_lua.c
#include "embedded_lua.h"
#include "hardware_bindings.h" // 你的硬件函数声明

lua_State* embedded_lua_init(void) {
    lua_State* L = luaL_newstate();
    if (!L) return NULL;
    
    // 1. 打开基础库（按需裁剪）
    luaopen_base(L);       // _G, 基础函数
    // luaopen_table(L);   // 按需
    // luaopen_string(L);
    // luaopen_math(L);
    
    // 2. 注册你的硬件API
    embedded_lua_register_function(L, "hw_led", lua_led);
    // ... 注册其他函数
    
    return L;
}

int embedded_lua_execute(lua_State* L, const char* script) {
    return luaL_dostring(L, script);
}

void embedded_lua_register_function(lua_State* L, const char* name, lua_CFunction func) {
    lua_register(L, name, func);
}

void embedded_lua_close(lua_State* L) {
    lua_close(L);
}