#ifndef __SERVER_LIB_H
#define __SERVER_LIB_H

#include <stdio.h>

void now(char *, size_t);

extern FILE *_log;
void LOG(char *, ...);

#endif
