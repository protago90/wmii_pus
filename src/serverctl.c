#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <stdio.h>
#include <stdlib.h>

#define FNAMEMAX 80
#define CMDMAX 256
#define OUTPUTMAXBUF 10240

int main() {
    char ctrl_fname[80];
    sprintf(ctrl_fname, "/run/user/%d/server", getuid());
    for(;;) {
        int sock = socket(AF_LOCAL, SOCK_STREAM, 0);
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_LOCAL;
        strcpy(addr.sun_path, ctrl_fname);
        if(connect(sock, (struct sockaddr *) &addr, sizeof(addr))) break;
        if(isatty(fileno(stdout))) {
            printf("%% "); fflush(stdout);
        }
        char cmd[CMDMAX];
        if(!fgets(cmd, CMDMAX, stdin)) break;
        write(sock, cmd, strlen(cmd));
        char buf[OUTPUTMAXBUF];
        int n = read(sock, buf, OUTPUTMAXBUF);
        if(n > 0) {
            write(1, buf, n);
        }
        close(sock);
    }
    return 0;
}