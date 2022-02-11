/*
 * test_data.cpp
 *
 *  Created on: Nov 18, 2016
 *      Author: sunchao
 */
/*  测试rawdata的打包解包操作：
 *  对字符串、数字等类型打包。
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils/rawdata.h"

static int test_rawdata()
{
    RawData raw(5);//新建任务，参数为缓冲区大小
    int a = 97;
    raw.pack("Hello World!", strlen("Hello World!"));//打包数据，内容，长度
    raw.pack(&a, sizeof(int));
    assert(memcmp("Hello World!a\0\0\0", raw.data(), raw.length()) == 0);
    raw.reset();//重启打包服务
    int num[4];
    raw.unpack(num, 4 * sizeof(int));//解包，参数为解包位置及数量
    assert(num[3] == a);
    return 0;
}

int main(int argc, char **argv)
{
    assert(!test_rawdata());
    printf("All Tests passed!\n");
    return 0;
}

