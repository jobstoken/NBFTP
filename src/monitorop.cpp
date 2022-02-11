/*
 * monitorop.cpp
 *
 *  Created on: Jan 11, 2017
 *      Author: sunchao
 */

#include <string.h>
#include <sys/stat.h>

#include "monitorop.h"
#include "taskop.h"
#include "utils/log.h"
#include "utils/config.h"
#include "utils/basic.h"
#include "utils/file.h"
#include "utils/monitor.h"

// 回调函数的参数列表
struct AddFileArg {
    char *path;         //test目录
    int compress;       //Compress
    int priority;       //Priority
    char *logfile;      //   test/.last_added
    long last_sec;      //
    long last_nsec;     //
};

static Monitor *monitors = NULL;
static int monitor_len = 0;

//filename就是要传输的文件的文件名,文件夹内出现文件加入，就执行回调函数，将文件传输任务加入传输队列
static void add_file(const char *filename, void *_arg) {
    AddFileArg *arg = (AddFileArg *)_arg;
    if (filename == NULL) {
        delete arg->path;
        delete arg->logfile;
        delete arg;
        return;
    }

    struct stat file_stat;
    char *full_path = stradd(arg->path, "/", filename);//拼成一个完整的文件(路径)
    stat(full_path, &file_stat);
    arg->last_sec = file_stat.st_mtim.tv_sec;
    arg->last_nsec = file_stat.st_mtim.tv_nsec;
    //if (arg->compress == 1) {
    //    gzip_compress(full_path);
    //    full_path = stradd(full_path, ".gz");
    //}

    //将文件传输的任务加入文件传输队列中
    add_file_task(&send_queue, full_path, arg->priority);
    delete full_path;
    File log_fp(arg->logfile, "w+");
    log_fp.print("%ld %ld\n", arg->last_sec, arg->last_nsec);
    log_fp.close();
}

static void confirm_file(const char *filename, void *_arg) {
    AddFileArg *arg = (AddFileArg *)_arg;
    struct stat file_stat;
    char *full_path = stradd(arg->path, "/", filename);//test文件夹下已有文件的全路径名
    stat(full_path, &file_stat);
    delete full_path;
    if (arg->last_sec < file_stat.st_mtim.tv_sec)
        add_file(filename, _arg);
    else if (arg->last_sec == file_stat.st_mtim.tv_sec && arg->last_nsec < file_stat.st_mtim.tv_nsec )
        add_file(filename, _arg);
}

void init_monitors() {
    stop_monitors();
    log.info("[MONITOR] Initialize monitor module.");
    char *names = strdup(config.get_string("TASK", "Monitors", ""));//MON1
    if (strlen(names) == 0) return;
    monitor_len = strcount(names, ';') + 1;
    monitors = new Monitor[monitor_len];
    char *name, *delim = names;
    for (int i = 0; i < monitor_len; i++) {
        name = delim;
        delim = strchr(delim, ';');//第一次出现;的位置，没有返回NULL
        if (delim != NULL) *delim = '\0';
        const char *path = config.get_string(name, "Directory", "");//test文件夹
        const char *pattern = config.get_string(name, "Pattern", "");
        if (strlen(path) <= 0) continue;
        if (!is_exist(path)) continue;
        if (strlen(pattern) <= 0) continue;

        AddFileArg *arg = new AddFileArg;
        if (path[strlen(path) - 1] == '/') {
            arg->path = strbase(path, '/');
        } else {
            arg->path = strdup(path);
        }
        arg->compress = config.get_int(name, "Compress", 0);
        arg->priority = config.get_int(name, "Priority", 5);
        arg->logfile = stradd(arg->path, "/", ".last_added");
        File log_fp(arg->logfile, "r");
        if (log_fp.is_null()) {
            arg->last_sec = 0;
            arg->last_nsec = 0;
        } else {
            log_fp.scan("%ld%ld", &arg->last_sec, &arg->last_nsec);
        }
        //设置要监控的文件夹 test文件夹
        monitors[i].set(path, pattern, add_file, arg);//arg->addfile
        // 检查已有文件(test文件夹中)是否都已加入传输队列（被记录）没加入的就加入 判断一下改动时间
        monitors[i].walk(confirm_file, arg);
    }
    delete names;
}

void check_monitors() {
    if (monitors == NULL) return;
    for (int i = 0; i < monitor_len; i++) {
        monitors[i].check();
    }
}

void stop_monitors() {
    log.info("[MONITOR] Stop monitor module.");
    if (monitors == NULL) return;
    for (int i =  0; i < monitor_len; i++) {
        monitors[i].stop();
    }
    monitor_len = 0;
    monitors = NULL;
}
