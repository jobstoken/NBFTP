/*
 * protect.cpp
 *
 *  Created on: Jan 10, 2017
 *      Author: sunchao
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "protect.h"
#include "control_part.h"
#include "utils/config.h"
#include "utils/log.h"

const char *main_pidfile = "/var/run/nbftp.pid";
const char *protect_pidfile = "/var/run/nbftp_protect.pid";
const char *run_status = "/var/run/nbftp.run";
static const char *cur_pidfile = NULL;

// 主程序的运行参数
static char **process_argv = NULL;
// 保护进程发送心跳的时间
static time_t last_status = 0;

static void heartbeat_sig(int sig) {
    // 记录保护进程通信的时间
    last_status = time(NULL);
}

static void new_io() {
    for (int i = 3; i < 256; i++)
        close(i);
    dup2(open("/dev/null", O_RDONLY), 0);
    dup2(open("/dev/null", O_WRONLY), 1);
    dup2(open("/dev/null", O_WRONLY), 2);
}

static void pidfile_delete(void) {
    if (cur_pidfile) unlink(cur_pidfile);
}

static int get_pid(const char *pidfile) {
    int pid = -1;
    FILE *pid_fp = fopen(pidfile, "r");
    if (pid_fp == NULL) {
        log.info("[PROTECT] CANNOT open pid file %s.", pidfile);
    } else {
        if (fscanf(pid_fp, "%d", &pid) != 1) {
            pid = -1;
        }
        fclose(pid_fp);
    }
    return pid;
}

static void protect_main() {
    while (is_exist(run_status)) {
        if (config.get_int("PROTECT", "Using", 0) == 0) return;
        int main_pid = get_pid(main_pidfile);
        if (main_pid < 0 || kill(main_pid, SIGUSR1) < 0) {
            if (is_exist(run_status)) {
                pid_t cid = fork();
                if (cid == 0) {
                    new_io();
                    log.info("[PROTECT] [MAIN] Start main process with PID %d.", getpid());
                    char *argv[3] = {process_argv[0], "--server", NULL};
                    execv(argv[0], argv);
                    exit(1);
                }
            }
        }
        int period = config.get_int("PROTECT", "Period", 30);
        for (int i = 0; i < period && is_exist(run_status); i++) {
            usleep(1000000);
        }
    }
}

void init_protect(char **argv) {
    signal(SIGCHLD, SIG_IGN);//父进程忽略对子进程的处理，交由内核来处理。
    signal(SIGUSR1, heartbeat_sig);
    process_argv = argv;
    log.debug("[PROTECT] Initialize protect module.");
    create_file(run_status);
    if (strcmp(argv[1], "--server") == 0) {
        save_pidfile(main_pidfile);
        last_status = time(NULL);
    } else if (strcmp(argv[1], "--protect") == 0) {
        save_pidfile(protect_pidfile);
        protect_main();
        exit(0);
    }
}

void save_pidfile(const char *pidfile) {
    int _pid = get_pid(pidfile);
    if (_pid > 0) {
        if (kill(_pid, SIGUSR1) >= 0) {
            // 已经有另一个进程在运行。
            exit(-1);
        }
    }

    int pid_fd = open(pidfile, O_CREAT | O_WRONLY, 0644);
    if (pid_fd < 0) exit(-1);

    atexit(pidfile_delete);
    cur_pidfile = pidfile;
    FILE *out = fdopen(pid_fd, "w");
    if (out != NULL) {
        fprintf(out, "%d\n", getpid());
        fclose(out);
    }
    close(pid_fd);
}

void check_protect() {
    if (config.get_int("PROTECT", "Using", 0) == 0) return;

    int timeout = config.get_int("PROTECT", "Timeout", 10);
    int cur_time = time(NULL);
    if (last_status + timeout < cur_time) {
        pid_t cid = fork();
        if (cid == 0) {
            new_io();
            log.info("[PROTECT] [CHECK] Start protect process with PID %d.", getpid());
            char *argv[3] = {process_argv[0], "--protect", NULL};
            execv(argv[0], argv);
            exit(1);
        } else {
            last_status = cur_time;
        }
    }
}

void stop_protect() {
    delete_file(run_status);
}
