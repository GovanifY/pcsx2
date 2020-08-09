#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include "Common.h"
#include "IPC.h"


SocketIPC::SocketIPC() {
}

void SocketIPC::Start() {
    struct sockaddr_un server;

    m_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_sock < 0) {
	    Console.WriteLn( Color_Red, "IPC: Cannot open socket! Shutting down...\n" );
        return;
    }
    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, SOCKET_NAME);

    // we unlink the socket so that when releasing this thread the socket gets
    // freed even if we didn't close correctly the loop
    unlink(SOCKET_NAME);
    if (bind(m_sock, (struct sockaddr *) &server, sizeof(struct sockaddr_un))) {
	    Console.WriteLn( Color_Red, "IPC: Error while binding to socket! Shutting down...\n" );
        return;
    }

    // maximum queue of 5 commands before refusing
    listen(m_sock, 5);
    // TODO: start thread here
    SocketThread();
}

void SocketIPC::SocketThread() {
    char buf[1024];

    while(true) {
        m_msgsock = accept(m_sock, 0, 0);
        if (m_msgsock == -1) {
	            Console.WriteLn( Color_Red, "IPC: Connection to socket broken! Shutting down...\n" );
                return;
            }
        else {
            bzero(buf, sizeof(buf));
            if (read(m_msgsock, buf, 1024) < 0) {
	            Console.WriteLn( Color_Red, "IPC: Connection to socket broken! Shutting down...\n" );
                return;
            }
            else {
                ParseCommand(buf);
            }
        }
    }
}
void SocketIPC::Stop() {
    // TODO: stop thread here
    close(m_msgsock);
    close(m_sock);
    unlink(SOCKET_NAME);

}
SocketIPC::~SocketIPC() {
    Stop();
}

char* SocketIPC::ParseCommand(char *buf) {
    // TODO: Actually parse different IPC events
    printf("-->%s\n", buf);
    int opcode = (int)buf[0];
    switch (opcode) {
        case MsgRead8: 
            printf("read 8!"); 
            break;
        case MsgRead16: 
            printf("read 16!"); 
            break;
        case MsgRead32: 
            printf("read 32!"); 
            break;
        case MsgRead64: 
            printf("read 64!"); 
            break;
        case MsgWrite8: 
            printf("write 8!"); 
            break;
        case MsgWrite16: 
            printf("write 16!"); 
            break;
        case MsgWrite32: 
            printf("write 32!"); 
            break;
        case MsgWrite64: 
            printf("write 64!"); 
            break;
    }
    return buf;
}
