#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "server_lib.h"

void now(char *buf, size_t len) {
    time_t now = time(0);
    strftime(buf, len, "%Y-%m-%d %H:%M:%S", localtime(&now));
}

FILE *_log;
void LOG(char *format, ...) {
    char buf[40];
    now(buf, sizeof(buf));
    fprintf(_log, "%s ", buf);
    va_list args;
    va_start(args, format);
    vfprintf(_log, format, args);
    va_end(args);
    fprintf(_log, "\n");
    fflush(_log);
}