/*
 * dataop.cpp
 *
 *  Created on: Jan 11, 2017
 *      Author: sunchao
 */

#include <stdlib.h>
#include <string.h>
#include "zlib.h"

#include "dataop.h"
#include "utils/file.h"
#include "utils/config.h"
#include "utils/log.h"
#include "utils/basic.h"

/* 加密数据。 */
static RawData *encrypt(RawData *data) {
    return data;
}

/* 解密数据。 */
static RawData *decrypt(RawData *data) {
    return data;
}

/* 压缩数据。 */
// Return RawData---success, NULL---failed
// Returned RawData: COMPRESS_DATA + COMPRESSED CONTENT, with _length setted.
static RawData *compress(RawData *data) {
    uLong ori_len = data->length();
    uLong comp_len = compressBound(ori_len);
    char *buffer = new char[comp_len];
    int err = compress((Bytef *)buffer + 1, &comp_len, (Bytef *)data->data(), ori_len);
    if (err != Z_OK || ori_len < comp_len + 5) {
        log.error("[DATAOP] Compress error: %d.", err);
        delete buffer;
        return data;
    }

    log.debug("[DATAOP] Compress success, size in byte: %ld -> %ld.", ori_len, comp_len+5);
    //压缩过的数据也是封装到一个新的rawdata内，接收端会根据类型字段对接受的数据进行相应处理
    RawData *comp_data = new RawData(comp_len + 32);
    comp_data->pack_char(COMPRESS_DATA);
    comp_data->pack_int(ori_len);
    comp_data->pack(buffer, comp_len);
    delete data;
    return comp_data;
}

/* 解压缩数据。 */
// Return data---success, NULL---failed
// Input RawData: COMPRESS_DATA + COMPRESSED CONTENT
// Return RawData: Original content, with _length setted.
static RawData *decompress(RawData *data) {
    uLong ori_len = data->unpack_int();
    char *ori_buf = new char[ori_len];
    uLong comp_len = data->length() - data->pos();
    char *comp_data = data->data() + data->pos();
    int err = uncompress((Bytef *)ori_buf, &ori_len, (const Bytef*)comp_data, comp_len);
    if (err != Z_OK) {
        log.error("[DATAOP] Decompress error: %d.", err);
        delete ori_buf;
        return data;
    }
    log.debug("[DATAOP] Decompress success, size in byte: %ld -> %ld.", comp_len, ori_len);
    RawData *ori_data = new RawData(comp_data, comp_len);
    delete data;
    return ori_data;
}

RawData *presend_data(RawData *data) {
    if (config.get_int("DATA", "Compress", 0) == 1) {
        data = compress(data);
    }
    if (config.get_int("DATA", "Encrypt", 0) == 1) {
        data = encrypt(data);
    }
    return data;
}

RawData *postrecv_data(RawData *data) {
    while (true) {
        data->reset();
        DataType type = (DataType)data->unpack_char();
        if (type == CRYPT_DATA) {
            data = decrypt(data);
        } else if (type == COMPRESS_DATA) {
            data = decompress(data);
        } else {
             break;
        }
    }
    return data;
}

