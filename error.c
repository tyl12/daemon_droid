//
// Created by david on 7/3/18.
//

#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zconf.h>
#include <errno.h>
#include "error.h"

#define MAX_CHARS 128

static void err_doit(int errnoflag, int errnum, const char *fmt, va_list args)
{
    char buf[MAX_CHARS];

    vsnprintf(buf, MAX_CHARS - 1, fmt, args);
    if (errnoflag)
    {
        snprintf(buf + strlen(buf), MAX_CHARS - strlen(buf) - 1, ": %s", strerror(errnum));
    }

    strcat(buf, "\n");
    fflush(stdout);

    fputs(buf, stderr);
    fflush(NULL);
}

void err_quit(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    err_doit(0, 0, fmt, args);
    va_end(args);

    exit(1);
}

void err_sys(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    err_doit(1, errno, fmt, args);
    va_end(args);

    exit(1);
}
