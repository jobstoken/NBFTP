/*
 * extension.cpp
 *
 *  Created on: Jan 14, 2017
 *      Author: sunchao
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "extension.h"
#include "utils/basic.h"
#include "utils/log.h"
#include "utils/config.h"

struct Extension {
    char *parttern;
    char *command;
};

static int extension_len;
static Extension *extensions;

static char *format(const char *str, const char *path_or_info, const char *truename) {
    char *str1 = strreplace(str, "%{fullpath}s", path_or_info);
    char *str2 = strreplace(str1, "%{truename}s", truename);
    char *str3 = strreplace(str2, "%{message}s", path_or_info);
    delete str1;
    delete str2;
    return str3;
}


void init_extension() {
    stop_extension();
    log.info("[EXTENSION] Initialize extension module.");
    char *names = strdup(config.get_string("DATA", "Extension", ""));
    if (strlen(names) == 0) return;

    extension_len = strcount(names, ';') + 1;
    extensions = new Extension[extension_len];
    char *name, *delim = names;
    for (int i = 0; i < extension_len; i++) {
        name = delim;
        delim = strchr(delim, ';');
        if (delim != NULL) *(delim++) = '\0';
        if (strlen(name) <= 0) {
            extension_len --, i--;
            continue;
        }
        extensions[i].parttern = strdup(config.get_string(name, "Pattern", "*"));
        extensions[i].command = strdup(config.get_string(name, "Command", "echo"));
    }
    delete names;
}

void call_extension(const char *type, const char *path_or_info, const char *truename) {
    log.debug("[EXTENSION] Call extension for %s%s.", type, path_or_info);
    for (int i = 0; i < extension_len; i++) {
        if (strfit(truename, extensions[i].parttern) != 0) {
            continue;
        }
        log.debug("[EXTENSION] Call command %s %s.", extensions[i].command, path_or_info);
        char *command = format(extensions[i].command, path_or_info, truename);
        int ret = system(command);
        if (ret != 0) {
            log.error("[EXTENSION] Doing command error: %s.", command);
        }
        delete command;
    }
}

void stop_extension() {
    log.info("[EXTENSION] Stop extension module.");
    if (extensions == NULL) return;
    for (int i = 0; i < extension_len; i++) {
        delete extensions[i].parttern;
        delete extensions[i].command;
    }
    delete extensions;
    extension_len = 0;
    extensions = NULL;
}
