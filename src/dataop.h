/*
 * dataop.h
 *
 *  Created on: Jan 11, 2017
 *      Author: sunchao
 */

#ifndef DATAOP_H_
#define DATAOP_H_

#include "utils/rawdata.h"

/* 发送数据前的准备，在配置文件中定义，可能包括加密压缩等。 */
RawData *presend_data(RawData *data);

/* 接收数据后的预处理，可能需要解密解压缩。 */
RawData *postrecv_data(RawData *data);

#endif /* DATAOP_H_ */
