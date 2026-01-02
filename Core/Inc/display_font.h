#pragma once

#include "stdint.h"
#include <stdint.h>

// !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~
//↑这里有空格

#define FONT_TYPE_ASCII 437
#define FONT_TYPE_GBK   936

typedef struct _pFont
{    
	const uint8_t 		*pTable;  		//  字模数组地址
	uint16_t 			Width; 		 	//  单个字符的字模宽度
	uint16_t 			Height; 		//  单个字符的字模长度
	uint16_t 			Sizes;	 		//  单个字符的字模数据个数
	uint16_t			Table_Rows;		//  该参数只有汉字字模用到，表示二维数组的行大小
    uint16_t            FontType;       //  字体标识符
} pFONT;

/*  ASCII Font  */
extern pFONT ASCII_8x16;
extern pFONT ASCII_9x18;
extern pFONT ASCII_10x20;
