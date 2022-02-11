/*
 * rawdata.cpp
 *
 *  Created on: Dec 19, 2016
 *      Author: sunchao
 */

#include <stdlib.h>
#include <string.h>
#include "rawdata.h"

#define CACHE_SIZE 4096

RawData::RawData(int size) {
    _size = size;
    _length = 0;
    _pos = 0;
    if (_size > 0) _data = new char[_size];
    else _data = NULL;
}

RawData::RawData(char *data, int length) {
    _size = length;
    _length = length;
    _pos = 0;
    _data = data;
}

RawData::~RawData() {
    if (_data != NULL) {
        delete _data;
        _data = NULL;
    }
}

//重新分配内存，将_data的数据拷贝到新分配的空间里
void RawData::realloc(int size) {
    do {
        _size += CACHE_SIZE;
    } while (_size < size);
    char *_new = new char[_size];
    if (_data != NULL) {
        memcpy(_new, _data, _length);
        delete _data;
    }
    _data = _new;
}

int RawData::pos() {
    return _pos;
}

void RawData::set_pos(int pos) {
    _pos = pos;
}

int RawData::length() {
    return _length;
}

char *RawData::data() {
    return _data;
}

void RawData::pack(const void *data, int length) {
    if (_pos + length > _size) {//超过缓冲上限就重新分配空间
        realloc(_pos + length);
    }
    memcpy(_data + _pos, data, length);//数据拷贝
    _pos += length;
    if (_pos > _length) _length = _pos;
}

void RawData::pack_char(char value) {
    pack(&value, sizeof(char));
}

void RawData::pack_int(int value) {
    pack(&value, sizeof(int));
}

void RawData::pack_long(long value) {
    pack(&value, sizeof(long));
}

void RawData::pack_string(const char *data) {
    int length = strlen(data) + 1;
    pack(&length, sizeof(int));
    pack(data, length);
}

void RawData::pack_file(File *fp, long length) {
    pack(&length, sizeof(long));
    if (_pos + length > _size) {
        realloc(_pos + length);
    }
    fp->read(_data + _pos, 1, length);
    _pos += length;
    if (_pos > _length) _length = _pos;
}

void RawData::unpack(void *data, int length) {
    if (_pos + length > _length) return;//超出上线就不读了
    memcpy(data, _data + _pos, length);//读出数据
    _pos += length;//更新偏移量
}

char RawData::unpack_char() {
    char value = 0;
    unpack(&value, sizeof(char));
    return value;
}

int RawData::unpack_int() {
    int value = 0;
    unpack(&value, sizeof(int));
    return value;
}

long RawData::unpack_long() {
    long value = 0;
    unpack(&value, sizeof(long));
    return value;
}

char *RawData::unpack_string(char *data) {
    int length = unpack_int();
    if (data == NULL) {
        data = new char[length + 1];
    }
    unpack(data, length);
    return data;
}

long RawData::unpack_file(File *fp) {
    long length = unpack_long();
    if (_pos + length > _length) return 0;
    fp->write(_data + _pos, 1, length);
    _pos += length;
    return length;
}

void RawData::reset() {
    _pos = 0;
}
