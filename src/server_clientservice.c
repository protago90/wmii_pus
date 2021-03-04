#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "server.h"
#include "server_lib.h"
#include "server_clientservice.h"
#include "server_sqlite.h"

void *subclientservice(void *raw_arg) {
    struct clientservicearg *arg = (struct clientservicearg *) raw_arg;
    FILE *out = fdopen(arg->csock, "w");
    setbuf(out, NULL);
    fprintf(out, "/connection ok\n");

    for(;;) {
        sleep(1);
        if(arg->login_id) {
            int msg_id;
            while(msg_id = getMessage(arg->login_id)) {
                char buf_t[40];
                char buf_from[20];
                char buf_msg[MAXINPUT];
                readMessageAndMeta(msg_id, buf_msg, buf_t, buf_from);
                fprintf(out, "/data %s %s\n%s\n", buf_t, buf_from, buf_msg);
                markMessage(msg_id);
            }
        }
    }
}

void *clientservice(void *raw_arg) {
    nclients++;
    struct clientservicearg *arg = (struct clientservicearg *) raw_arg;
    LOG("connection(%d) from %d.%d.%d.%d established", arg->index, arg->ip[0], arg->ip[1], arg->ip[2], arg->ip[3]);

    // client service
    FILE *in = fdopen(arg->csock, "r");
    FILE *out = fdopen(arg->csock, "w");
    setbuf(out, NULL);

    pthread_t tidread;
    pthread_create(&tidread, NULL, subclientservice, raw_arg);

    for(;;) {
        char input[MAXINPUT];
        if(!fgets(input, MAXINPUT, in)) break;

        int n = strlen(input);
        if(input[0] == '\n') continue;
        if(input[n - 1] == '\n') input[n - 1] = 0;

        if(n > 0 && input[0] == '/') {
            char cmd[MAXINPUT] = "", arg1[MAXINPUT] = "", arg2[MAXINPUT] = "", arg3[MAXINPUT] = "";
            int nw = sscanf(input, "%s %s %s %s", cmd, arg1, arg2, arg3);
            if(nw > 0) {
                if(!strcmp(cmd + 1, "login")) {
                    if(strlen(arg1) > 0) {
                        int id;
                        if((id = checkUser(arg1, arg2)) > 0) {
                            arg->login_id = id;
                            strncpy(arg->login, arg1, MAXLOGIN + 1);
                            LOG("user %s logged in using connection(%d)", arg->login, arg->index);
                            fprintf(out, "/login ok %s\n", arg->login);
                        } else {
                            LOG("authentication error for user %s using connection(%d)", arg1, arg->index);
                            fprintf(out, "/login failed %s\n", arg1);
                        }
                    } else {
                        fprintf(out, "/login as %s\n", arg->login);
                    }
                } else if(!strcmp(cmd + 1, "time")) {
                    char buf[40];
                    now(buf, sizeof(buf));
                    fprintf(out, "/time %s\n", buf);
                } else if(!strcmp(cmd + 1, "logout")) {
                    arg->login_id = 0;
                    arg->login[0] = 0;
                    fprintf(out, "/logout ok\n");
                } else if(!strcmp(cmd + 1, "list")) {
                    fprintf(out, "/list begin\n");
                    for(int i = 0; i < MAXCLIENTS; i++) {
                        if(clients[i] && clients[i]->login[0]) {
                            fprintf(out, "%s", clients[i]->login);
                            if(clients[i] == arg) {
                                fprintf(out, " -> %s", arg->sendto);
                            }
                            fprintf(out, "\n");
                        }
                    }
                    fprintf(out, "/list end\n");
                } else if(!strcmp(cmd + 1, "sendto")) {
                    if(strlen(arg1) > 0) {
                        int id;
                        if((id = getUser(arg1)) > 0) {
                            arg->sendto_id = id;
                            strncpy(arg->sendto, arg1, MAXLOGIN + 1);
                            fprintf(out, "/sendto set %s\n", arg->sendto);
                        } else {
                            fprintf(out, "/sendto failed %s\n", arg1);
                        }
                    } else {
                        fprintf(out, "/sendto is %s\n", arg->sendto);
                    }

                } else {
                    fprintf(out, "/error cmd %s\n", cmd);
                }
            }

        } else {
            if(!arg->login_id) {
                fprintf(out, "/error not-logged-in\n");
                continue;
            }
            if(!arg->sendto_id) {
                fprintf(out, "/error sendto-not-set\n");
                continue;
            }
            storeMessage(arg->login_id, arg->sendto_id, input);
            fprintf(out, "/sent %d\n", n);
        }
    }

    close(arg->csock);
    LOG("connection(%d) from %d.%d.%d.%d closed", arg->index, arg->ip[0], arg->ip[1], arg->ip[2], arg->ip[3]);
    clients[arg->index] = NULL;
    free(raw_arg);
    nclients--;
}