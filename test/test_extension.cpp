/*
 * test_extension.cpp
 *
 *  Created on: Jan 19, 2017
 *      Author: sunchao
 */
/*  测试扩展模块，扩展模块为多线程实现，而在使用时完全屏蔽。
 *  初始化扩展模块
 *  对扩展模块的接口进行调用
 *  清理资源，实现隐藏等待任务完成。
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "utils/config.h"
#include "utils/log.h"
#include "extension.h"

void test_extension() {
    init_extension();
    call_extension("FILE", "build/test_extension", "test_extension");
    call_extension("FILE", "build/utils", "utils.ex");
    call_extension("MESSAGE", "Message: Test Complete!", "MESSAGE");
    stop_extension();
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Need a parameter of configuration filename!");
        exit(1);
    }
    char *filename = argv[1];
    config.init(filename);
    const char *log_dir = config.get_string("LOG", "Directory", "build/logfiles_test");
    log.init(log_dir);

    test_extension();
    return 0;
}

