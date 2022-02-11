/*
 * nbftp.cpp
 *
 *  Created on: Jan 12, 2017
 *      Author: sunchao
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "control_part.h"

void print_help(int exit_code) {
    printf("Usage: default port is 9998\n");
    printf("\tnbftp [-p port] stop\n");
    printf("\tnbftp [-p port] push FILENAME [PRIORITY]\n");
    printf("\tnbftp [-p port] mail MESSAGE\n");
    printf("\tnbftp [-p port] pull TASKID\n");
    printf("\tnbftp [-p port] pull all\n");
    printf("\tnbftp [-p port] pending\n");
    printf("\tnbftp [-p port] list\n");
    printf("\tnbftp [-p port] get TASKID\n");
    printf("\tnbftp [-p port] delete TASKID\n");
    printf("\tnbftp [-p port] delete all\n");
    exit(exit_code);
}

static int parse_result(RawData *response) {
    if (response == NULL) {
        fprintf(stderr, "Options or parameters Error!\n");
        exit(1);
    }
    char *message = new char[response->length()];
    while (response->pos() < response->length() - 4) {
        response->unpack_string(message);
        printf("%s\n", message);
    }
    delete message;
    return response->unpack_int();
}

int main(int argc, char **argv) {
    argc--, argv++;
    if (argc < 1 || strcmp(argv[0], "-h") == 0) {
        print_help(0);
    }

    int port = 9998;
    if (strcmp(argv[0], "-p") == 0) {
        port = atoi(argv[1]);
        argc -= 2, argv += 2;
    }
    Control control(port);
    RawData *response = NULL;
    if (strcmp(argv[0], "stop") == 0) {
        response = control.stop_server();
    } else if (strcmp(argv[0], "push") == 0) {
        if (argc == 3) {
            response = control.add_send_file(argv[1], atoi(argv[2]));
        } else if (argc == 2) {
            response = control.add_send_file(argv[1], 5);
        }
    } else if (strcmp(argv[0], "mail") == 0) {
        if (argc == 2) {
            response = control.add_send_message(argv[1], 3);
        }
    } else if (strcmp(argv[0], "pull") == 0) {
        if (argc == 2) {
            if (strcmp(argv[1], "all") == 0) {
                response = control.delete_send_task(-1);
            } else {
                response = control.delete_send_task(atol(argv[1]));
            }
        }
    } else if (strcmp(argv[0], "pending") == 0) {
        response = control.list_send_tasks();
    } else if (strcmp(argv[0], "list") == 0) {
        response = control.list_received_tasks();
    } else if (strcmp(argv[0], "get") == 0) {
        if (argc == 2) {
            response = control.get_received_task(atol(argv[1]));
        }
    } else if (strcmp(argv[0], "delete") == 0) {
        if (argc == 2) {
            if (strcmp(argv[1], "all") == 0) {
                response = control.delete_received_task(-1);
            } else {
                response = control.delete_received_task(atol(argv[1]));
            }
        }
    }
    int ret = parse_result(response);
    delete response;
    return ret;
}
