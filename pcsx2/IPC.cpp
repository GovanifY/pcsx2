#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include "Common.h"
#include "IPC.h"


namespace SocketIPC {

void SocketThread() {
    int sock, msgsock, rval;
    struct sockaddr_un server;
    char buf[1024];

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
	    Console.WriteLn( Color_StrongBlue, "IPC: Cannot open socket! Shutting down...\n" );
        return;
    }
    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, NAME);

    // we unlink the socket so that when releasing this thread the socket gets
    // freed even if we didn't close correctly the loop
    unlink(NAME);
    if (bind(sock, (struct sockaddr *) &server, sizeof(struct sockaddr_un))) {
	    Console.WriteLn( Color_StrongBlue, "IPC: Error while binding to socket! Shutting down...\n" );
        return;
    }

    // maximum queue of 5 commands before refusing
    listen(sock, 5);
    // TODO: start thread here
    SocketThread();
}

void SocketThread() {
    while(true) {
        msgsock = accept(sock, 0, 0);
        if (msgsock == -1) {
	            Console.WriteLn( Color_StrongBlue, "IPC: Connection to socket broken! Shutting down...\n" );
                return;
            }
        else do {
            bzero(buf, sizeof(buf));
            if ((rval = read(msgsock, buf, 1024)) < 0) {
	            Console.WriteLn( Color_StrongBlue, "IPC: Connection to socket broken! Shutting down...\n" );
                return;
            }
            else {
                ParseCommand(buf);
            }
        } 
        close(msgsock);
    }
    // catch signal here
    close(sock);
    unlink(NAME);

}

void ParseCommand(char *buf) {
    // TODO: Actually parse different IPC events
    printf("-->%s\n", buf);
}

} // namespace SocketIPC
