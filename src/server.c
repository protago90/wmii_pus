#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "server.h"
#include "server_lib.h"
#include "server_clientservice.h"
#include "server_sqlite.h"

static char ctrl_fname[80];

void clean(int sig) {
    dbclose();
    unlink(ctrl_fname);
    exit(0);
}

int nclients = 0;
struct clientservicearg *clients[MAXCLIENTS]; // NULL means "no client"

void *tcpserver(void *arg) {
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP), opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    if(bind(sock, (struct sockaddr *) &addr, sizeof(addr))) {
        LOG("bind to port %d/tcp failed", PORT);
        exit(2);
    }
    listen(sock, 5);
    for(;;) {
        struct sockaddr_in clientaddr;
        unsigned int sockaddr_size = sizeof(clientaddr);
        int csock = accept(sock, (struct sockaddr *) &clientaddr, &sockaddr_size);
        pthread_t tid;
        struct clientservicearg *arg = (struct clientservicearg *) calloc(1, sizeof(struct clientservicearg));
        arg->index = -1;
        int i;
        for(i = 0; i < MAXCLIENTS; i++) {
            if(!clients[i]) {
                arg->index = i;
                clients[i] = arg;
                break;
            }
        }
        if(i == MAXCLIENTS) {
            LOG("connection refused, max clients reached");
            free(arg);
            FILE *out = fdopen(csock, "w");
            fprintf(out, "/connection failed\n");
            fclose(out);
            close(csock);
        } else {
            arg->csock = csock;
            memcpy(arg->ip, &clientaddr.sin_addr, 4);
            pthread_create(&tid, NULL, clientservice, arg);
        }
    }
}

void *udpserver(void *arg) {
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    if(bind(sock, (struct sockaddr *) &addr, sizeof(addr))) {
        LOG("bind to port %d/udp-server failed", PORT);
        exit(2);
    }
    for(;;) {
        struct sockaddr_in clientaddr;
        unsigned int sockaddr_size = sizeof(clientaddr);
        
        char ingram[DGRAMMAX];
        // char ingram_fb[DGRAMMAX];
        int n = recvfrom(sock, ingram, sizeof(ingram), 0, (struct sockaddr *) &clientaddr, &sockaddr_size);
        if (n > 0) {
            unsigned char *ip = (unsigned char *) &clientaddr.sin_addr;
            if(ingram[n - 1] == '\n') ingram[n - 1] = 0;
            char incmd[MAXINPUT];
            char msg_host[64]; 
            char msg_data[MAXINPUT];
            int msg_date; 
            int msg_from; 
            int msg_to;
            int nw = sscanf(ingram, "%s %d;%[^;];%d;%d;%[^;]", incmd, &msg_date, msg_host, &msg_from, &msg_to, msg_data);
            if(!strcmp(incmd, "migration")) {
                if (nw == 6){
                    LOG("udp %d-bytes datagram received from %d.%d.%d.%d", n, ip[0], ip[1], ip[2], ip[3]);
                    storeMessageAndMeta(msg_from, msg_to, msg_date, msg_data, msg_host);
                    // snprintf(ingram_fb, sizeof(ingram_fb), "migrationdone %d\n", msg_date);
                    // sendto(sock, ingram_fb, sizeof(ingram_fb), 0, (const struct sockaddr *) &clientaddr, sockaddr_size);
                    // LOG("udp migrationdone %d\n", msg_date);
                    // sendto(sock, msg_date, sizeof(msg_date), 0, (const struct sockaddr *) &clientaddr, sockaddr_size);
                }
            // } else if (!strcmp(incmd, "migrationdone")){
            //      markMessage(msg_date);
            }
        }

        char outdgram[DGRAMMAX];
        int msg_id; 
        while(msg_id = getMigration()) {
            char msg_migration[MAXINPUT];
            unsigned char *ip = (unsigned char *) &clientaddr.sin_addr;
            readMigration(msg_id, msg_migration);
            snprintf(outdgram, sizeof(outdgram), "migration %s", msg_migration);
            sendto(sock, outdgram, sizeof(outdgram), 0, (const struct sockaddr *) &clientaddr, sockaddr_size);
            sendto(sock, "pst", 3, 0, (const struct sockaddr *) &clientaddr, sockaddr_size);
            LOG("udp datagram send %s %d.%d.%d.%d", msg_migration, ip[0], ip[1], ip[2], ip[3]);
            markMigration(msg_id);
        }

        sleep(1);
        sendto(sock, "ping", 4, 0, (const struct sockaddr *) &clientaddr, sockaddr_size);
    }
}

int main() {
    _log = fopen(LOGFNAME, "a");
    if(!_log) {
        _log = stderr;
        LOG("unable to open log file %s", LOGFNAME);
    } else
        daemon(1, 0); // no chdir
    dbconnect();
    LOG("server started, pid=%d", getpid());

    pthread_t nstid;
    pthread_create(&nstid, NULL, tcpserver, NULL);
    pthread_create(&nstid, NULL, udpserver, NULL);
    signal(SIGINT, clean);
    signal(SIGTERM, clean);
    signal(SIGQUIT, clean);

    sprintf(ctrl_fname, CTRL_PATTERN, getuid());
    unlink(ctrl_fname);
    int ctrlsock = socket(AF_LOCAL, SOCK_STREAM, 0);
    struct sockaddr_un addr;
    addr.sun_family = AF_LOCAL;
    strcpy(addr.sun_path, ctrl_fname);
    if(bind(ctrlsock, (struct sockaddr *) &addr, sizeof(addr))) {
        LOG("unable to create control socket %s", ctrl_fname);
        pause();
    } else {
        listen(ctrlsock, 5);
        for(;;) {
            int csock = accept(ctrlsock, NULL, NULL);
            char cmdline[CMDMAX];
            memset(cmdline, 0, CMDMAX);
            int n = read(csock, cmdline, CMDMAX);
            if(n > 0) {
                if(cmdline[n - 1] == '\n') cmdline[n - 1] = 0;
                LOG("command from ctrl socket: %s", cmdline);
                char cmd[CMDMAX] = "", arg1[CMDMAX] = "", arg2[CMDMAX] = "", arg3[CMDMAX] = "";
                int nw = sscanf(cmdline, "%s %s %s %s", cmd, arg1, arg2, arg3);
                if(nw > 0) {
                    FILE *ctrl = fdopen(csock, "w");
                    if(!strcmp(cmd, "shutdown")) {
                        fprintf(ctrl, "server disconnected\n"); fflush(ctrl);
                        clean(0);
                    } else if(!strcmp(cmd, "info")) {
                        fprintf(ctrl, "getpid() = %d\n", getpid());
                        fprintf(ctrl, "nclients = %d\n", nclients);
                        for(int i = 0; i < MAXCLIENTS; i++) {
                            if(clients[i]) {
                                fprintf(ctrl, "%d\t%d.%d.%d.%d\t%s\n",
                                  i,
                                  clients[i]->ip[0], clients[i]->ip[1], clients[i]->ip[2], clients[i]->ip[3],
                                  clients[i]->login);
                            }
                        }
                        fflush(ctrl);
                    } else if(!strcmp(cmd, "help")) {
                        fprintf(ctrl, "Help\n\nThere is a place for help\n"); fflush(ctrl);
                    } else {
                        fprintf(ctrl, "%s not implemented\n", cmd); fflush(ctrl);
                    }
                    fclose(ctrl);
                }
            }
            close(csock);
        }
    }
    return 0;
}