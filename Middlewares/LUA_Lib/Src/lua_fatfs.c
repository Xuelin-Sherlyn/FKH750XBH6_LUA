#include "lua_fatfs.h"
#include "ff.h"
#include "lua.h"
#include "string.h"
#include "fatfs.h"
#include "usart.h"
#include <string.h>

FATFS *fs;

// 1. 注册函数结构体数组（便于批量注册）
const luaL_Reg fs_functions[] = {
    {"mount", lua_fatfs_mount},
    {"umount", lua_fatfs_umount},
    {"ls", lua_fatfs_ls},
    {"rm", lua_fatfs_rm},
    {"mkdir", lua_fatfs_mkdir},
    {"rmdir", lua_fatfs_rmdir},
    {"getfree", lua_fatfs_getfree},
    {"help", lua_fatfs_help},
    {NULL, NULL} // 结束标记
};

int luaopen_fatfs(lua_State *L)
{
    // 创建 fatfs 表
    luaL_newlib(L, fs_functions);
    return 1;
}

// 3. 初始化函数（在你的main.c或嵌入式初始化中调用）
void fatfs_bindings_init(lua_State* L) {
    // 注册硬件模块为全局表 "fatfs"
    luaL_requiref(L, "fatfs", luaopen_fatfs, 1);
    lua_pop(L, 1); // 移除require留下的副本
}

int lua_fatfs_mount(lua_State *L)
{
    BYTE work[_MAX_SS];

	retSD = f_mount(&SDFatFS, SDPath, 1);	 // 挂载SD卡

	if (retSD == FR_OK)	//判断是否挂载成功
	{
		safe_printf("\r\n\033[32mSD File System Mount Sucess\033[0m\r\n");
        return 0;
	}
	else
	{
		if(retSD == 13)
		{
			safe_printf("\r\033[33mSD Card`s File system has not been created yet, about to format\033[0m\r\n");

			retSD = f_mkfs(SDPath,FM_FAT32,0,work,sizeof(work));		//格式化SD卡，FAT32，簇默认大小16K

			if (retSD == FR_OK)		//判断是否格式化成功
            {
                return 0;
                safe_printf("\r\033[32mSD card formatted successfully!\033[0m\r\n");
            }
			else
            {
                return lua_error(L);
                safe_printf("\r\033[31mFormatting failed, please check or replace the SD card!\033[0m\r\n");
            }
		}
		else
        {
            safe_printf("\r\033[31mMount Fail:%d\033[0m\r\n",retSD);
            return lua_error(L);
        }
	}
}

int lua_fatfs_umount(lua_State *L)
{
    retSD = f_mount(NULL, SDPath, 1);
    if(retSD != FR_OK)
    {
        safe_printf("\r\n\033[31mUmount fail\033[0m\r\n");
        return 1;
    }
    safe_printf("\r\n\033[32mUmount sucess\033[0m\r\n");
    return 0;
}

int lua_fatfs_ls(lua_State *L)
{
    FRESULT res; //文件操作返回代码
	DIR dir;     //目录对象
	static FILINFO fno; //文件信息句柄
    const char* path = NULL;

    int argc = lua_gettop(L);
    if(argc != 1)
    {
        lua_pushstring(L, "Just need 1 arguments!");
        return lua_error(L);
    }
    if(!lua_isstring(L, 1))
    {
        lua_pushstring(L, "Arguments must be string!");
        return lua_error(L);
    }
    path = lua_tostring(L, 1);

    if(path != NULL)
    {
        safe_printf("\r\nTarget Media Path:%s\r\n", path);
	    res = f_opendir(&dir, path); //打开目录
	    if (res == FR_OK)
	    {
		    while (1) //循环读取
		    {
			    res = f_readdir(&dir, &fno);
			    if (res != FR_OK || fno.fname[0] == 0) break;
			    char path_pic[512];
			    sprintf(path_pic, "%s/%s", path, fno.fname);
			    if(f_stat(path_pic, &fno) != FR_OK)
                {
                    safe_printf("\rGet info fail\r\n");
                    return 1;
                }
                if (fno.fattrib & AM_DIR)
                    safe_printf("\r<Dir>  %s\r\n", path_pic);
                else
                    safe_printf("\r<File> %s\r\n", path_pic);
		    }
		    f_closedir(&dir); //关闭目录
	    }
	    else
	    {
		    safe_printf("\ropen Fail: %d\r\n", res); //输出失败信息
	    }
    }
    else 
    {
        safe_printf("\rTarget Media Path is NULL!\r\n"); 
    }
    return 0;
}

int lua_fatfs_rm(lua_State *L)
{
	static FILINFO fno; //文件信息句柄
    const char* path = NULL;

    int argc = lua_gettop(L);
    if(argc != 1)
    {
        lua_pushstring(L, "Just need 1 arguments!");
        return lua_error(L);
    }
    if(!lua_isstring(L, 1))
    {
        lua_pushstring(L, "Arguments must be string!");
        return lua_error(L);
    }
    path = lua_tostring(L, 1);

    if(path != NULL)
    {
        if(f_stat(path, &fno) != FR_OK)
        {
            safe_printf("\rGet info fail\r\n");
            return 1;
        }
        if (fno.fattrib & AM_DIR)
        {
            lua_pushstring(L, "Please use 'rmdir' remove folder");
            return lua_error(L);
        }
        if(f_unlink(path) != FR_OK)
        {
            lua_pushstring(L, "Remove file fail");
            return lua_error(L);
        }
        safe_printf("\r\n\033[32mSucess remove file\033[0m\r\n");
        return 0;
    }
    return 1;
}

int lua_fatfs_mkdir(lua_State *L)
{
    int argc = lua_gettop(L);
    if(argc != 1)
    {
        lua_pushstring(L, "Just need 1 arguments!");
        return lua_error(L);
    }
    if(!lua_isstring(L, 1))
    {
        lua_pushstring(L, "Arguments must be string!");
        return lua_error(L);
    }
    const char *path = luaL_checkstring(L, 1);
    FRESULT res = f_mkdir(path);
    
    if (res != FR_OK) {
        safe_printf("\rmkdir Fail: %d\r\n", res);
        return 1;
    }
    return 0;
}

int lua_fatfs_rmdir(lua_State *L)
{
    static FILINFO fno; //文件信息句柄
    const char* path = NULL;

    int argc = lua_gettop(L);
    if(argc != 1)
    {
        lua_pushstring(L, "Just need 1 arguments!");
        return lua_error(L);
    }
    if(!lua_isstring(L, 1))
    {
        lua_pushstring(L, "Arguments must be string!");
        return lua_error(L);
    }
    path = lua_tostring(L, 1);

    if(path != NULL)
    {
        if(f_stat(path, &fno) != FR_OK)
        {
            safe_printf("\rGet info fail\r\n");
            return 1;
        }
        if (fno.fattrib & AM_DIR){
            if(f_unlink(path) != FR_OK)
            {
                lua_pushstring(L, "Remove folder fail");
                return lua_error(L);
            }
            safe_printf("\r\n\033[32mSucess remove file\033[0m\r\n");
            return 0;
        }
        else {
            lua_pushstring(L, "Please use 'rm' remove file");
            return lua_error(L);
        }
    }
    return 1;
}

int lua_fatfs_getfree(lua_State *L)
{
    uint32_t SD_CardCapacity = 0;		//SD卡的总容量
	uint32_t SD_FreeCapacity = 0;		//SD卡空闲容量
	DWORD fre_clust, fre_sect, tot_sect; 	//空闲簇，空闲扇区数，总扇区数

	retSD = f_getfree(SDPath,&fre_clust,&fs);			//获取SD卡剩余的簇
    if(retSD != FR_OK)
    {
        safe_printf("\r\n\033[31mGet free space fail\033[0m\r\n");
        return 1;
    }
    tot_sect = (fs->n_fatent-2) * fs->csize;	//总扇区数量 = 总的簇 * 每个簇包含的扇区数
	fre_sect = fre_clust * fs->csize;			//计算剩余的可用扇区数

	SD_CardCapacity = tot_sect / 2048 ;	// SD卡总容量 = 总扇区数 * 512( 每扇区的字节数 ) / 1048576(换算成MB)
	SD_FreeCapacity = fre_sect / 2048 ;	//计算剩余的容量，单位为M
	safe_printf("\r\n-------------------Get device capacity information-----------------\r\n");
	safe_printf("\rSD Capacity:%luMB\r\n", (unsigned long)SD_CardCapacity);
	safe_printf("\rSD Remaining:%luMB\r\n", (unsigned long)SD_FreeCapacity);
    return 0;
}

int lua_fatfs_help(lua_State* L)
{
    safe_printf("\r\n\033[36m=== FatFS API Help ===\033[0m\r\n");
    safe_printf("fatfs.mount()                               - Mount drivers\r\n");
    safe_printf("fatfs.umount()                              - Umount drivers\r\n");
    safe_printf("fatfs.ls(<(string)Path>)                    - List the directory\r\n");
    safe_printf("fatfs.rm(<(string)File Path>)               - Remove File\r\n");
    safe_printf("fatfs.mkdir(<(string)Folder Path>)          - Make the Directory\r\n");
    safe_printf("fatfs.rmdir(<(string)Folder Path>)          - Remove Directory\r\n");
    safe_printf("fatfs.getfree(<(string)Path>)               - Get drivers free space\r\n");
    // 更多帮助信息...
    safe_printf("\r\nType 'fatfs' to see available functions\r\n");
    return 0;
}