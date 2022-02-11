/*
 * nbftp_server.cpp
 *
 *  Created on: Dec 22, 2016
 *      Author: sunchao
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>

#include "taskop.h"
#include "communication.h"
#include "control_part.h"
#include "extension.h"
#include "monitorop.h"
#include "protect.h"
#include "utils/log.h"
#include "utils/config.h"
#include "utils/transfer.h"

void start_server() {
    int port = config.get_int("MAIN", "Port", 9999);//传输数据的端口
    int sock_num = config.get_int("MAIN", "MaxLink", 5);
    int reuse_on = config.get_int("MAIN", "ReuseOn", 1);
    Socket socket(port, sock_num, reuse_on);    // 启动监听服务

    while (status == 1) {
        check_monitors();       // 检查文件系统变化
        Connection *conn = socket.client_link();    // 客户端连接
        if (conn == NULL)  {
            usleep(999999);
            continue;
        }
        if (conn->connected()) {
            int timeout = config.get_int("MAIN", "Timeout", 5);
            conn->set_timeout(timeout);         // 设置超时时间
            if (send_tasks(conn) == -1) {
                log.error("[MAIN] Send tasks error!");
            } else {
                log.debug("[MAIN] Send end, begin receive.");
                receive_tasks(conn);  // 接收数据
            }
            conn->close_conn();
        } else {
            log.error("[MAIN] Connection colsed!");
        }
        delete conn;
    }
    status = -1;
}

void start_client() {
    char *host = strdup(config.get_string("MAIN", "Host", "127.0.0.1"));
    int port = config.get_int("MAIN", "Port", 9999);

    while (status == 1) {
        check_monitors();       // 检查文件系统变化
        Connection *conn = new Connection(host, port);  // 连接服务器
        if (conn != NULL)  {
            if (conn->connected()) {
                int timeout = config.get_int("MAIN", "Timeout", 5);
                conn->set_timeout(timeout);         // 设置超时时间
                if (receive_tasks(conn) == -1) {//接收数据
                    log.error("[MAIN] Receive tasks error!");
                } else {
                    log.debug("[MAIN] Receive end, begin send.");
                    send_tasks(conn);  // 发送数据
                }
                conn->close_conn();
            } else {
                log.error("[MAIN] Connection colsed!");
            }
            delete conn;
        }
        int interval = config.get_int("MAIN", "Interval", 600);
        for (int i = 0; i < interval && status == 1; i++) {
            usleep(1000000);
        }
    }
    status = -1;
}

void *start_control(void *arg) {
    int port = config.get_int("CONTROL", "Port", 9998);//接收控制命令的端口
    Control control(port);
    control.server_routine(check_protect);
    return NULL;
}

int main(int argc, char **argv) {
    const char *config_path = "/etc/nbftp/nbftp.conf";
    if (argc >= 2) {
        if (argv[1][0] == '-') { // --server or --protect
            daemon(0, 0);//第一个0表示当前位置变为根目录，输入输出重定向
        } else {
            config_path = argv[1];//命令行参数设置配置文件路径
        }
    }
    config.init(config_path);  // 初始化配置文件
    const char *log_dir = config.get_string("LOG", "Directory", "/var/log/nbftp");
    log.init(log_dir);      // 初始化日志模块
    log.setLogLevel(config.get_int("LOG", "Level", 7));

    signal(SIGPIPE, SIG_IGN);    // 网络连接断开的处理,忽略网络断开,不因客户端断开而终止进程
    init_protect(argv);     // 初始化交叉保护
    init_taskqueues();      // 初始化发送接收队列
    init_monitors();        // 初始化系统监控模块 监控文件变动
    init_extension();       // 初始化扩展模块

    pthread_t control_pid;
    pthread_create(&control_pid, NULL, start_control, NULL);
    if (config.get_int("MAIN", "Server", 1) == 1) {
        start_server();
    } else {
        start_client();
    }
    pthread_join(control_pid, NULL);

    stop_extension();           // 停止扩展模块
    stop_monitors();            // 停止文件监控模块
    stop_protect();             // 停止交叉保护
    return 0;
}
