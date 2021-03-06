#ifndef __SERVER_H
#define __SERVER_H

#include "server_clientservice.h"

#define MAXCLIENTS 1024
#define LOGFNAME "/tmp/server.log"
#define CTRL_PATTERN "/run/user/%d/server"

#define FNAMEMAX 80
#define CMDMAX 256
#define PORT 9999
#define DGRAMMAX 65536

extern struct clientservicearg *clients[MAXCLIENTS];

#endif