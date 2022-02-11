/*
 * monitor.cpp
 *
 *  Created on: Jan 10, 2017
 *      Author: sunchao
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/inotify.h>

#include "monitor.h"
#include "log.h"
#include "basic.h"
#include "file.h"

Monitor::Monitor() {
    _monitor = inotify_init();
    if (_monitor < 0) {
        log.error("[MONITOR] CANNOT start monitor!");
    }
    _watcher = -1;
    _path = NULL;
    _pattern = NULL;
    _notity = NULL;
    _params = NULL;
}

Monitor::~Monitor() {
    stop();
    if (_monitor >= 0) {
        close(_monitor);
        _monitor = -1;
    }
}

void Monitor::set(const char *path, const char *pattern, notify_t notify, void *params) {
    if (_monitor < 0) return;
    stop();
    uint32_t watch_mask = IN_MOVED_TO | IN_CLOSE_WRITE;//设置监控的类型标志
    _watcher = inotify_add_watch(_monitor, path, watch_mask);//添加监控
    if (_watcher < 0) {
        log.error("[MONITOR] CANNOT start watcher!");
    } else {
        _path = strdup(path);
        _pattern = strdup(pattern);
        _notity = notify;
        _params = params;
        log.info("[MONITOR] Start monitor on %s for pattern(%s).", _path, _pattern);
    }
}

void Monitor::walk(notify_t notify, void *params) {
    DIR *dir = opendir(_path);//已经在test文件中的文件
    if (dir == NULL) return;
    dirent *ptr = readdir(dir);
    while (ptr != NULL) {
        if (ptr->d_type == 8) {
            if (ptr->d_name[0] == '.') {
                log.info("[MONITOR] Automatic ignore hidden file %s/%s.", _path, ptr->d_name);
            } else if (strfit(ptr->d_name, _pattern) == 0) {
                (*notify)(ptr->d_name, params);// 传入confirm_file已有文件的文件名和参数
            }
        }
        ptr = readdir(dir);
    }
    closedir(dir);
}

//查看所监控的文件是否发生变动，有变动就调用回调函数重新加载
void Monitor::check() {
    log.debug("[MONITOR] begin checking ...");
    if (_watcher < 0) return;
    int buffer_size = 0;
    if (ioctl(_monitor, FIONREAD, &buffer_size) < 0) {//从_monitor中读，到输入缓冲区，读到多少放到b_s里，成功返回0，失败-1.
        log.debug("[MONITOR] FIONREAD!");
        return;
    }
    log.debug("[MONITOR] Buffer size: %d", buffer_size);
    int esize = sizeof(inotify_event);//一个事件的大小
    if (buffer_size < esize) return;//根据比较大小判断是不是一个完整的文件改动事件
    char *buffer = new char[buffer_size];
    int actual_read = 0;
    while (actual_read < buffer_size) {//buffer_size可能一次性读不完
        actual_read += read(_monitor, buffer + actual_read, buffer_size - actual_read);
    }
    for (int pos = 0; pos < actual_read;) {
        inotify_event *event = (inotify_event *)(buffer + pos);//先取一个事件
        pos += sizeof(inotify_event) + event->len;//一个事件的真正大小，根据之前读到事件大小来确定位置
        if (event->len <= 0) continue;

        if ((event->mask & (IN_MOVED_TO | IN_CLOSE_WRITE)) == 0) {
            log.error("[MONITOR] Unknown Monitor Mask: %s/%s %d.", _path, event->name, event->mask);
        } else {
            if (event->name[0] == '.') {//event->name 就是 event->mask 下的文件名
                log.info("[MONITOR] Automatic ignore hidden file %s/%s.", _path, event->name);
            } else if (strfit(event->name, _pattern) == 0) {//所监控的文件发生改变
                log.info("[MONITOR] File Changed: %s/%s.", _path, event->name);
                (*_notity)(event->name, _params);//调用add_file将传输文件加入到传输队列中
            } else {
                log.debug("[MONITOR] Unhandled file changed: %s/%s.", _path, event->name);
            }
        }
    }
    delete buffer;
}

void Monitor::stop() {
    if (_monitor < 0) return;
    if (_watcher >= 0) {
        log.info("[MONITOR] Stop monitor on %s of pattern(%s).", _path, _pattern);
        inotify_rm_watch(_monitor, _watcher);
        _watcher = -1;
    }
    if (_pattern != NULL) {
        delete _pattern;
        _path = NULL;
        _pattern = NULL;
    }
    if (_path != NULL) {
        delete _path;
        _path = NULL;
    }
    if (_notity != NULL) {
        (*_notity)(NULL, _params);      // 清理_params
        _params = NULL;
        _notity = NULL;
    }
}
