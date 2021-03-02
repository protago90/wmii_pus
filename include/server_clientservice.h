#ifndef __SERVER_CLIENTSERVICE_H
#define __SERVER_CLIENTSERVICE_H

#include <bits/stdint-uintn.h>
#include <time.h>

#define MAXINPUT 1024
#define MAXLOGIN 15

extern int nclients;

struct clientservicearg {
    int index;
    int csock;
    uint8_t ip[4];
    int login_id;
    char login[MAXLOGIN + 1];
    int sendto_id;
    char sendto[MAXLOGIN + 1];
    time_t time_conn;
    time_t time_login;
};

void *clientservice(void *raw_arg);

#endif