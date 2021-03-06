/*
 * file.cpp
 *
 *  Created on: Dec 20, 2016
 *      Author: sunchao
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <openssl/md5.h>
#include "file.h"
#include "basic.h"
#include "log.h"

File::File(const char *filename, const char *modes) {
    if (strlen(filename) > 0) {
        while (1) {
            _fp = fopen(filename, modes);
            if (_fp == NULL) {
                if (!is_exist(filename)) {
                    log.error("[FILE] File %s NOT exist!", filename);
                    break;
                }
            } else {
                break;
            }
            usleep(999999);
            log.info("[FILE] File %s CANNOT open! try again ...", filename);
        }
    } else {
        _fp = NULL;
    }
}

//打开配置文件，创建文件结构体变量
File::File(const char *dir, const char *filename, const char *modes) {
    char *full_path = NULL;
    if (dir == NULL) {
        full_path = strdup(filename);
    } else {
        full_path = stradd(dir, "/", filename);
    }
    while (1) {
        _fp = fopen(full_path, modes);//核心就是打开文件，返回一个file结构体变量
        if (_fp == NULL) {
            if (!is_exist(full_path)) {
                log.error("[FILE] File %s NOT exist!", full_path);
                break;
            }
        } else {
            break;
        }
        usleep(999999);
        log.info("[FILE] File %s CANNOT open! try again ...", full_path);
    }
    delete full_path;
}

File::~File() {
    close();
}

bool File::is_null() {
    return _fp == NULL;
}

bool File::is_eof() {
    if (_fp == NULL) return true;
    else return feof(_fp) != 0;
}

size_t File::size() {
    if (is_null()) return 0;
    long current_pos, size;
    current_pos = ftell(_fp);
    fseek(_fp, 0, SEEK_END);
    size = ftell(_fp);
    fseek(_fp, current_pos, SEEK_SET);
    return size;
}

void File::seek(long pos) {
    if (is_null()) return;
    fseek(_fp, pos, SEEK_SET);
}

int File::read(void *data, size_t size, size_t count) {
    if (is_eof()) return -1;
    size_t actual_read = fread(data, size, count, _fp);
    if (actual_read < count) {
        if (is_eof()) return -1;
        fprintf(stderr, "Read file ERROR!\n");
        exit(101);
    }
    return actual_read;
}

void File::readline(char *data, size_t size) {
    if (is_eof()) return;
    data = fgets(data, size, _fp);
}

void File::scan(const char *msg, ...) {
    if (is_eof()) return;
    va_list args;
    va_start(args, msg);
    int ret = vfscanf(_fp, msg, args);
    va_end(args);
    if (ret > 0) return;
}

void File::write(const void *data, size_t size, size_t count) {
    if (is_null()) return;
    size_t actual_write = fwrite(data, size, count, _fp);
    if (actual_write < count) {
        fprintf(stderr, "Write file ERROR!\n");
        exit(102);
    }
}

void File::print(const char *msg, ...) {
    if (is_null()) return;
    va_list args;
    va_start(args, msg);
    vprint(msg, args);
    va_end(args);
}

void File::vprint(const char *msg, va_list args) {
    if (is_null()) return;
    vfprintf(_fp, msg, args);
}

void File::get_md5(char *md5) {
    long left, current_pos;
    char buffer[4096];
    MD5_CTX ctx;

    if (is_null()) return;
    left = size();
    current_pos = ftell(_fp);
    fseek(_fp, 0, SEEK_SET);
    MD5_Init(&ctx);
    for (; left > 0; left -= 4096) {
        int len = left < 4096 ? left : 4096;
        read(buffer, 1, len);
        MD5_Update(&ctx, buffer, len);
    }
    MD5_Final((unsigned char *)md5, &ctx);
    fseek(_fp, current_pos, SEEK_SET);
}

int File::check_md5(char *md5) {
    char cal_md5[16];
    get_md5(cal_md5);
    int ret = memcmp(cal_md5, md5, 16);
    if (ret == 0) {
        log.info("[CHECK] check_md5() successful.");
        return 1;
    } else {
        log.error("[CHECK] check_md5() failed!");
        return 0;
    }
}

void File::flush() {
    if (is_null()) return;
    fflush(_fp);
}

void File::close() {
    if (is_null()) return;
    fclose(_fp);
    _fp = NULL;
}

int is_exist(const char *path) {
    return access(path, F_OK) != -1;
}

//创建目录
int create_dir(const char *dir) {
    if (is_exist(dir)) return 0;
    log.debug("[FILE] Create directory: %s.", dir);
    int ret = system_call("mkdir -p %s", dir);
    if (ret != 0) log.error("[File] Create directory %s ERROR!", dir);
    return ret;
}

int create_file(const char *filename) {
    if (is_exist(filename)) return 0;
    int ret = 0;
    char *dir = strbase(filename, '/');
    if (strlen(dir) > 0) ret = create_dir(dir);
    delete dir;
    if (ret == 0) {
        log.debug("[FILE] Create file: %s.", filename);
        ret = system_call("touch %s", filename);
        if (ret != 0) {
            log.error("[File] Create file %s ERROR!", filename);
        }
    }
    return ret;
}

int file_mode(const char *filename, int mode) {
    chmod(filename, mode);
    return 0;
}

int delete_file(const char *filename) {
    log.debug("[FILE] Delete file: %s.", filename);
    int ret = remove(filename);
    if (ret != 0) log.error("[FILE] Delete file %s ERROR!", filename);
    return ret;
}

int copy_file(const char *source, const char *target) {
    log.debug("[FILE] Copy file: %s -> %s.", source, target);
    int fin = open(source, O_RDONLY);
    int fout = open(target, O_WRONLY | O_CREAT, 0644);
    int ret = (fin < 0 || fout < 0) ? -1 : 0;
    int len;
    char buffer[4096];
    while ((len = read(fin, buffer, 4096)) > 0) {
        if (write(fout, buffer, len) < 0) {
            ret = -1;
            break;
        }
    }
    if (fin > 0) close(fin);
    if (fout > 0) close(fout);
    if (ret < 0) {
        log.error("[FILE] Copy file %s -> %s ERROR!", source, target);
        exit(255);
    }
    return ret;
}

int rename_file(const char *filename, const char *newname) {
    log.debug("[FILE] Rename file: %s -> %s.", filename, newname);
    int ret = rename(filename, newname);
    if (ret != 0) log.error("[FILE] Rename file: %s -> %s ERROR!", filename, newname);
    return ret;
}

int gzip_compress(const char *filename) {
    log.debug("[FILE] GZIP compress file: %s.", filename);
    int ret = system_call("gzip -qf '%s'", filename);
    if (ret != 0) log.error("[FILE] Compress file %s error!", filename);
    return ret;
}
