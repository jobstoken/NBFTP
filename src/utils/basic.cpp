/*
 * basic.cpp
 *
 *  Created on: Jan 12, 2017
 *      Author: sunchao
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "basic.h"

int system_call(const char *__restrict cmd, ...) {
    char buffer[CMD_LEN];
    va_list args;
    va_start(args, cmd);
    vsprintf(buffer, cmd, args);//其实就是拼成一个命令
    va_end(args);
    return system(buffer);//系统执行
}

char *stradd(const char *__restrict a, const char *__restrict b) {
    char *buffer = new char[strlen(a) + strlen(b) + 1];
    sprintf(buffer, "%s%s", a, b);
    return buffer;
}

//将参数拼成一个文件的路径
char *stradd(const char *__restrict a, const char *__restrict b, const char *__restrict c) {
    char *buffer = new char[strlen(a) + strlen(b) + strlen(c) + 1];
    sprintf(buffer, "%s%s%s", a, b, c);
    return buffer;
}

//从输入的文件路径分割出目录
char *strbase(const char *__restrict str, char chr) {
    const char *pos = strrchr(str, chr);//最后一次出现chr的位置,没找到返回NULL，如果是根目录下的就没有目录
    if (pos == NULL) return strdup("");
    else return strndup(str, pos - str);
}

//从输入的文件路径分割出文件
char *strext(const char *__restrict str, char chr) {
    const char *pos = strrchr(str, chr);
    if (pos == NULL) return strdup(str);
    else return strdup(pos + 1);
}

int strcount(const char *__restrict str, char chr) {
    int count = 0;
    const char *pos = strchr(str, chr);
    while (pos != NULL) {
        count++;
        pos = strchr(pos + 1, chr);
    }
    return count;
}

char *strtrim(const char *__restrict str) {
    const char *begin = str;
    const char *end = str + strlen(str);
    while (begin < end) {
        if (isspace(*begin)) begin ++;
        else if (isspace(*(end-1))) end --;
        else break;
    }
    if (begin == end) return strdup("");
    else return strndup(begin, end - begin);
}

int strfit(const char *__restrict str, const char *__restrict pattern) {
    if (strlen(pattern) == 0) {
        return strlen(str);
    } else if (pattern[0] == '*') {
        if (strlen(pattern) == 1) {
            return 0;
        }
        else {
            for (const char *pos = str; *pos != 0; pos++) {
                if (strfit(pos, pattern + 1) == 0) {
                    return 0;
                }
            }
            return -1;
        }
    } else if (str[0] == pattern[0]) {
        return strfit(str + 1, pattern + 1);
    } else {
        return -1;
    }
}

char *strreplace(const char *__restrict str, const char *__restrict from, const char *__restrict to) {
    const char *find = strstr(str, from);
    if (find == NULL) return strdup(str);

    char *pre_str = strndup(str, find - str);
    char *post_str = strreplace(find + strlen(from), from, to);
    char *buffer = new char[strlen(pre_str) + strlen(to) + strlen(post_str) + 1];
    sprintf(buffer, "%s%s%s", pre_str, to, post_str);
    delete pre_str;
    delete post_str;
    return buffer;
}

// 将str中前length个字符转化为16进制
char* str2hex(const char *str, long length){
    // Must initial to 0 here!
    char *buffer = new char[length * 2 + 1]();
    for (int i = 0; i < length; i++) {
        sprintf(buffer + i * 2, "%02X", str[i]);
    }
    return buffer;
}
