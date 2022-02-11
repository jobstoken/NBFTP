/*
 * rawdata.h
 *
 *  Created on: Dec 19, 2016
 *      Author: sunchao
 */

#ifndef RAWDATA_H_
#define RAWDATA_H_

#include "file.h"

enum DataType:char {
    SMALL_FILE_DATA,       // 对应任务数据格式
    LARGE_FILE_BEGIN,
    MESSAGE_DATA,
    MD5_CHECK_DATA,
    LARGE_FILE_PART = 16,  // 之后为特殊数据格式(范围是16..31)
    CRYPT_DATA,
    COMPRESS_DATA
};

class RawData {
private:
    int _size;       // 缓冲区大小
    int _pos;        // 当前数据位置
    int _length;     // 实际数据长度
    char *_data;        // 实际数据

    /* 重新分配缓冲区，增大缓冲空间。 */
    void realloc(int size);

public:
    /* 根据缓冲区大小初始化。用于打包待发送的数据包。 */
    RawData(int size);
    /* 根据已有数据初始化。用于解析接收到的数据包。 */
    RawData(char *data, int length);
    ~RawData();

    /* 当前数据位置。 */
    int pos();
    /* 设置当前数据位置。 */
    void set_pos(int pos);
    /* 。实际数据大小。 */
    int length();
    /* 实际数据。 */
    char *data();
    /* 重置当前数据位置。 */
    void reset();
    /* 打包数据到当前位置。 */
    void pack(const void *data, int length);
    /* 打包char值到当前位置。 */
    void pack_char(char value);
    /* 打包int值到当前位置。 */
    void pack_int(int value);
    /* 打包long值到当前位置。 */
    void pack_long(long value);
    /* 打包数据到当前位置。 */
    void pack_string(const char *data);
    /* 打包文件数据到当前位置。 */
    void pack_file(File *fp, long length);
    /* 从当前位置解析数据。 */
    void unpack(void *data, int length);
    /* 从当前位置解析char值。 */
    char unpack_char();
    /* 从当前位置解析int值。 */
    int unpack_int();
    /* 从当前位置解析long值。 */
    long unpack_long();
    /* 从当前位置解析数据。 */
    char *unpack_string(char *data);
    /* 从当前位置解析文件数据。 */
    long unpack_file(File *fp);
};

#endif /* RAWDATA_H_ */
