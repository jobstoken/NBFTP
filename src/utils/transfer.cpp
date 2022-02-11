/*
 * transfer.cpp
 *
 *  Created on: Dec 21, 2016
 *      Author: sunchao
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "protect.h"
#include "transfer.h"
#include "rawdata.h"
#include "log.h"

// 传输状态和控制，将其置为0则停止传送。-1为已经退出（在主程序中设置）。
int status = 1;

/* 非阻塞模式连接服务器。 */
// Rreturn 0---success, -1---failed
static int connect_nonb(int sockfd, struct sockaddr *addr, socklen_t length) {
    // Set nonblock in this behavior
    int flags = fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, flags) == -1) {
        log.error("[SOCKET] [client] Activate NONBLOCK error(%d): %s.", errno, strerror(errno));
        close(sockfd);
        return -1;
    }

    int ret = connect(sockfd, addr, length);
    if (ret < 0 && errno == EINPROGRESS) {
        fd_set wset;
        FD_ZERO(&wset);
        FD_SET(sockfd, &wset);
        struct timeval tval = {1, 0};
        do {
            ret = select(sockfd + 1, NULL, &wset, NULL, &tval);
        } while(ret < 0 && errno == EINTR);
        if (ret == 0) {
            log.error("[SOCKET] [client] Connect timeout.");
            return -1;
        } else if (ret == 1) {
            int err;
            socklen_t err_len = sizeof(err);
            ret = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &err_len);
            if (ret >= 0) {
                if (err == 0) ret = 0;
                else errno = err, ret = -1;
            }
        }
    }
    if (ret < 0) {
        log.error("[SOCKET] [client] Connect error(%d): %s.", errno, strerror(errno));
    } else {
        log.debug("[SOCKET] [client] Connect success.");
    }
    return ret;
}

static int readn(int fd, void *buf, size_t count, int timeout, int emergency) {
    int nread, err_cnt = 0, nleft = count;
    char *bufp = (char *)buf;//bufp指向的是一块内存，可以指向任何类型，int接收就是数据长度，指向数组就是接收这么长的数据
    while ((status == 1 || emergency == 1) && nleft > 0) {
        // read一次最多读4096字节，发来的RawData可能多于这个数，要循环读，读完为止
        if ((nread = read(fd, bufp, nleft)) < 0) {
            if (errno != EINTR && errno != EAGAIN) return -1;
            if (++ err_cnt > timeout * 10 || errno == EPIPE) return -2;
        } else if (nread == 0) {
            return -1;
        } else {
            err_cnt = 0;
            bufp += nread;
            nleft -= nread;
        }
        usleep(100000);
    }
    return (status == 1 || emergency == 1) ? count: -1;
}

static int writen(int fd, const void *buf, size_t count, int timeout, int emergency) {
    int nwritten, err_cnt = 0, nleft = count;
    char *bufp = (char *)buf;
    while ((status == 1 || emergency == 1) && nleft > 0) {
        if ((nwritten = write(fd, bufp, nleft)) < 0) {
            if (errno != EINTR && errno != EAGAIN) return -1;
            if (++ err_cnt > timeout * 10 || errno == EPIPE) return -2;
        } else if (nwritten > 0) {
            err_cnt = 0;
            bufp += nwritten;
            nleft -= nwritten;
        }
        usleep(100000);
    }
    return (status == 1 || emergency == 1) ? count: -1;
}

Connection::Connection(const int conn, const char *host) {
    _conn = conn;
    _host = strdup(host);//点分字节
    _timeout = 10;
    _pending = -1;
    log.info("[SOCKET] [server] Connected from %s.", _host);
}

Connection::Connection(const char *host, int port) {
    _host = strdup(host);
    _timeout = 10;
    _pending = -1;
    client_connect(port);
}

Connection::~Connection() {
    close_conn();
    log.debug("[SOCKET] ~Connection: %d, _host: %s", _conn, _host);
    if (_host != NULL) {
        delete _host;
        _host = NULL;
    }
}

void Connection::close_conn() {
    if (_conn >= 0) {
        close(_conn);
        log.info("[SOCKET] Close connection: %d.", _conn);
        _conn = -1;
    }
}

void Connection::client_connect(int port) {
    log.debug("[SOCKET] [client] client_connect() begin");
    _conn = socket(AF_INET, SOCK_STREAM, 0);
    if (_conn < 0) {
        log.error("[SOCKET] [client] Create socket error(%d): %s.", errno, strerror(errno));
        return;
    }
    sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_aton(_host, &addr.sin_addr) == 0) {
        log.error("[SOCKET] [client] Server %s is NOT valid.", _host);
        close_conn();
        return;
    }

    // Connect to server with non-block mode
    if (connect_nonb(_conn, (sockaddr *)&addr, sizeof(addr)) < 0) {
        close_conn();
        return;
    }
    log.info("[SOCKET] [client] Connected to server %s, _conn: %d.", _host, _conn);
}

bool Connection::connected(){
    return (_conn >= 0);
}

void Connection::set_timeout(int timeout) {
    _timeout = timeout;
}

int Connection::send_data(RawData *data) {
    return send_data(data, 0);
}

// Rreturn 0---success, -1---failed, -2---timeout 先发长度，再发数据
int Connection::send_data(RawData *data, int emergency) {
    if (_conn < 0) {
        log.error("[SOCKET] Send data error for NO connection!");
        return -1;
    }
    log.debug("[SOCKET] Pending DataLength: %d.", data->length());
    int data_len = data->length();
    int ret = writen(_conn, &data_len, sizeof(int), _timeout, emergency);
    if (ret < 0) {
        log.error("[SOCKET] Send socket error(%d): %s.", errno, strerror(errno));
        return ret;
    }
    ret = writen(_conn, data->data(), data->length(), _timeout, emergency);
    if (ret < 0) {
        log.error("[SOCKET] Send socket error(%d): %s.", errno, strerror(errno));
        return ret;
    }
    log.debug("[SOCKET] Send DataLength in Total: 4 + %d.", data->length());
    return 0;
}

int Connection::has_data() {
    return has_data(0);
}

int Connection::has_data(int emergency) {
    if (_conn < 0) {
        log.error("[SOCKET] [Part1] Receive data error for NO connection!");
        return -1;
    }
    int ret = readn(_conn, &_pending, sizeof(int), _timeout, emergency);
    if (ret < 0) {
        log.error("[SOCKET] [Part1] Receive socket error(%d): %s.", errno, strerror(errno));
        return ret;
    }
    log.info("[SOCKET] [Part1] Received Value: %d.", _pending);
    return 1;
}

RawData *Connection::recv_data() {
    return recv_data(0);
}

// Return RawData---success, NULL---failed
RawData *Connection::recv_data(int emergency) {
    if (_conn < 0) {
        log.error("[SOCKET] [Part2] Receive data error for NO connection!");
        return NULL;
    }
    int data_len = _pending;
    if (data_len < 0) return NULL;
    else if (data_len == 0) return new RawData(0);
    char *data = new char[data_len];
    if (readn(_conn, data, data_len, _timeout, emergency) < 0) {
        log.error("[SOCKET] [Part1] Receive socket error(%d): %s.", errno, strerror(errno));
        delete data;
        return NULL;
    }
    log.info("[SOCKET] Received DataLength in Total: %d.", 4 + data_len);
    _pending = -1;
    return new RawData(data, data_len);
}

Socket::Socket(int port, int sock_num) : _socket(-1) {
    start_server(port, sock_num, 1);
    log.info("[SOCKET] [server] Started socket server on port %d(_socket: %d).", port, _socket);//update by lxx
}

Socket::Socket(int port, int sock_num, int reuse_on) : _socket(-1) {
    start_server(port, sock_num, reuse_on);
    log.info("[SOCKET] [server] Started socket server on port %d(_socket: %d).", port, _socket);//update by lxx
}

Socket::~Socket() {
    if (_socket >= 0) {
        close(_socket);
        log.info("[SOCKET] [server] Stopped socket server(_socket: %d).\n", _socket);
        _socket = -1;
    }
}

void Socket::start_server(int port, int sock_num, int reuse_on) {
    int fd;
    sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    log.debug("[SOCKET] [server] start_server() begin");
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        log.error("[SOCKET] [server] Create socket server error(%d): %s.", errno, strerror(errno));
        return;
    }
    int flags = fcntl(fd, F_GETFL, 0) | O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1) {
        log.error("[SOCKET] [server] Activate NONBLOCK error(%d): %s.", errno, strerror(errno));
        close(fd);
        return;
    }
    
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse_on, sizeof(reuse_on)) < 0) {
        log.error("[SOCKET] [server] Set socket server option error(%s): %s.", errno, strerror(errno));
        close(fd);
        return;
    }
    if (bind(fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
        log.error("[SOCKET] [server] Bind socket server error(%d): %s.", errno, strerror(errno));
        close(fd);
        return;
    }
    if (listen(fd, sock_num) < 0) {
        log.error("[SOCKET] [server] Listen socket error(%d): %s.", errno, strerror(errno));
        close(fd);
        return;
    }
    _socket = fd;
    log.debug("[SOCKET] [server] Start server success.");
}

// Return Connection *---success, NULL---failed
Connection *Socket::client_link() {
    if (_socket < 0) {
        log.error("[SOCKET] Client link error for NO socket!");
        return NULL;
    }

    sockaddr_in addr;
    socklen_t length = sizeof(addr);
    log.debug("[SOCKET] [server] client_link() begin");
    int conn = accept(_socket, (sockaddr *) &addr, &length);
    if (conn < 0) {
        log.debug("[SOCKET] [server] No new client linking(%d): %s.", errno, strerror(errno));
        return NULL;
    }
    log.debug("[SOCKET] [server] Connected to client %s: %d.", inet_ntoa(addr.sin_addr), conn);

    int flags = fcntl(conn, F_GETFL, 0) | O_NONBLOCK;
    if (fcntl(conn, F_SETFL, flags) == -1) {
        log.error("[SOCKET] [server] Activate NONBLOCK error(%d): %s.", errno, strerror(errno));
        close(conn);
        return NULL;
    }
    return new Connection(conn, inet_ntoa(addr.sin_addr));
}
