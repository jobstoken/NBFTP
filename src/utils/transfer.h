/*
 * transfer.h
 *
 *  Created on: Dec 21, 2016
 *      Author: sunchao
 */

#ifndef TRANSFER_H_
#define TRANSFER_H_

#include "rawdata.h"

class Connection {
private:
    int _conn;              // Socket连接描述符
    char *_host;            // 对方主机名
    int _timeout;           // 传输超时时间
    int _pending;           // 等待接收的数据

    void client_connect(int port);

public:
    /* 通过已有的Socket连接描述符创建。host只用于记录信息。 */
    Connection(int conn, const char *host);
    /* 连接到host主机的port端口，并创建Socket连接描述符。 */
    Connection(const char *host, int port);
    /* 注销并关闭连接。 */
    ~Connection();

    /* 是否建立连接 */
    bool connected();
    /* 设置传输超时时间。 */
    void set_timeout(int timeout);
    /* 发送数据。如果发送成功返回0，失败返回-1。 */
    int send_data(RawData *data);
    int send_data(RawData *data, int emergency);
    /* 检查是否有接收的数据。如果有则返回1，如果没有返回0,出错返回-1。 */
    int has_data();
    int has_data(int emergency);
    /* 接收数据。如果接收成功返回数据指针，失败返回NULL。 */
    RawData *recv_data();
    RawData *recv_data(int emergency);
    /* 关闭连接。 */
    void close_conn();
};

class Socket {
private:
    int _socket;            // Socket服务器连接描述符

    void start_server(int port, int sock_num, int reuse_on);
public:
    /* 创建Socket服务器。监听port端口。 */
    Socket(int port, int sock_num);
    Socket(int port, int sock_num, int reuse_on);
    /* 关闭连接服务器。 */
    ~Socket();

    /* 接收客户端的连接。 */
    Connection *client_link();
};

/* 传输状态控制。 */
extern int status;

#endif /* TRANSFER_H_ */
