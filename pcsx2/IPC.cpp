
#include "PrecompiledHeader.h"

#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <sys/types.h>
#if _WIN32
#define bzero(b, len) (memset((b), '\0', (len)), (void)0)
#include <windows.h>
#else
#include <sys/socket.h>
#include <sys/un.h>
#endif

#include "Common.h"
#include "Memory.h"
#include "IPC.h"

// TODO: try-catch blocks do not avoid asserts/crash on linux when vm is
// inactive, fix that!!

SocketIPC::SocketIPC() : pxThread("IPC_Socket") {
#ifdef _WIN32
        WSADATA wsa;
        SOCKET new_socket;
        struct sockaddr_in server, client;
        int c;


        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            Console.WriteLn(Color_Red, "IPC: Cannot initialize winsock! Shutting down...");
            return;
        }

        //Create a socket
        if ((m_sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
            Console.WriteLn(Color_Red, "IPC: Cannot open socket! Shutting down...");
            return;
        }

        //Prepare the sockaddr_in structure
        server.sin_family = AF_INET;
        // localhost only
        server.sin_addr.s_addr = inet_addr("127.0.0.1");
        server.sin_port = htons(PORT);

        if (bind(m_sock, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
            Console.WriteLn(Color_Red, "IPC: Error while binding to socket! Shutting down...");
            return;
        }

#else
        struct sockaddr_un server;

        m_sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (m_sock < 0) {
            Console.WriteLn(Color_Red, "IPC: Cannot open socket! Shutting down...");
            return;
        }
        server.sun_family = AF_UNIX;
        strcpy(server.sun_path, SOCKET_NAME);

        // we unlink the socket so that when releasing this thread the socket gets
        // freed even if we didn't close correctly the loop
        unlink(SOCKET_NAME);
        if (bind(m_sock, (struct sockaddr*)&server, sizeof(struct sockaddr_un))) {
            Console.WriteLn(Color_Red, "IPC: Error while binding to socket! Shutting down...");
            return;
        }
#endif

        // maximum queue of 100 commands before refusing
        listen(m_sock, 100);

        // we start the thread
        Start();
}

void SocketIPC::ExecuteTaskInThread() {
    char buf[1024];
    int msgsock = 0;
    while (true) {
        msgsock = accept(m_sock, 0, 0);
        if (msgsock == -1) {
            return;
        }
        else {
            bzero(buf, sizeof(buf));
#ifdef _WIN32
            if (recv(msgsock, buf, 1024, 0) < 0) {
#else
            if (read(msgsock, buf, 1024) < 0) {
#endif
                    return;
                }
                else {
                    auto res = ParseCommand(buf);
#ifdef _WIN32
                    if (send(msgsock, res.second, res.first, 0) < 0) {
#else
                    if (write(msgsock, res.second, res.first) < 0) {
#endif
                        return;
                    }
                }
        }
    }
}

SocketIPC::~SocketIPC() {
#ifdef _WIN32
        closesocket(m_sock);
        WSACleanup();
#else
        close(m_sock);
        unlink(SOCKET_NAME);
#endif
        // destroy the thread
	    try {
		    pxThread::Cancel();
	    }
	    DESTRUCTOR_CATCHALL
}

char* SocketIPC::MakeOkIPC(int size) {
    char* res_array = (char*)malloc(size * sizeof(char));
    res_array[0] = (unsigned char) IPC_OK;
    return res_array;
}


char* SocketIPC::MakeFailIPC(int size) {
    char* res_array = (char*)malloc(size * sizeof(char));
    res_array[0] = (unsigned char) IPC_FAIL;
    return res_array;
}

char* SocketIPC::from64b(char* res_array, uint64_t res, int i) {
    res_array[i+0] = (unsigned char)(res >> 56) & 0xff;
    res_array[i+1] = (unsigned char)(res >> 48) & 0xff;
    res_array[i+2] = (unsigned char)(res >> 40) & 0xff;
    res_array[i+3] = (unsigned char)(res >> 32) & 0xff;
    res_array[i+4] = (unsigned char)(res >> 24) & 0xff;
    res_array[i+5] = (unsigned char)(res >> 16) & 0xff;
    res_array[i+6] = (unsigned char)(res >> 8) & 0xff;
    res_array[i+7] = (unsigned char)res;
    return res_array;
}

char* SocketIPC::from32b(char* res_array, uint32_t res, int i) {
    res_array[i+0] = (unsigned char)(res >> 24) & 0xff;
    res_array[i+1] = (unsigned char)(res >> 16) & 0xff;
    res_array[i+2] = (unsigned char)(res >> 8) & 0xff;
    res_array[i+3] = (unsigned char)res;
    return res_array;
}

char* SocketIPC::from16b(char* res_array, uint16_t res, int i) {
    res_array[i+0] = (unsigned char)(res >> 8) & 0xff;
    res_array[i+1] = (unsigned char)res;
    return res_array;
}

char* SocketIPC::from8b(char* res_array, uint8_t res, int i) {
    res_array[i+0] = (unsigned char)res;
    return res_array;
}


std::pair<int, char*> SocketIPC::ParseCommand(char* buf) {
    //         IPC Message event (1 byte)
    //         |  Memory address (4 byte)
    //         |  |           argument (VLE)
    //         |  |           |
    // format: XX YY YY YY YY ZZ ZZ ZZ ZZ
    //        reply code: 00 = OK, FF = NOT OK
    //        |  return value (VLE)
    //        |  |
    // reply: XX ZZ ZZ ZZ ZZ
    IPCCommand opcode = (IPCCommand)buf[0];
    // YY YY YY YY from schema above
    u32 a = to32b(&buf[1]);
    std::pair<int, char*> rval;
    switch (opcode) {
        case MsgRead8: {
                u8 res;
                try{ res = memRead8(a);}
                catch (...) { goto error; }
                rval = std::make_pair(2, from8b(MakeOkIPC(2), res, 1));
                break;
        }
        case MsgRead16: {
                u16 res;
                try{ res = memRead16(a);}
                catch (...) { goto error; }
                rval = std::make_pair(3, from16b(MakeOkIPC(3), res, 1));
                break;
        }
        case MsgRead32: {
                u32 res;
                try{ res = memRead32(a);}
                catch (...) { goto error; }
                rval = std::make_pair(5, from64b(MakeOkIPC(5), res, 1));
                break;
        }
        case MsgRead64: {
                u64 res;
                try{ memRead64(a, &res);}
                catch (...) { goto error; }
                rval = std::make_pair(9, from64b(MakeOkIPC(9), res, 1));
                break;
        }
        case MsgWrite8: {
                try{memWrite8(a, to8b(&buf[5]));}
                catch (...) { goto error; }
                rval = std::make_pair(1, MakeOkIPC(1));
                break;
        }
        case MsgWrite16: {
                try{memWrite16(a, to16b(&buf[5]));}
                catch (...) { goto error; }
                rval = std::make_pair(1, MakeOkIPC(1));
                break;
        }
        case MsgWrite32: {
                try{memWrite32(a, to32b(&buf[5]));}
                catch (...) { goto error; }
                rval = std::make_pair(1, MakeOkIPC(1));
                break;
        }
        case MsgWrite64: {
                try{memWrite64(a, to64b(&buf[5]));}
                catch (...) { goto error; }
                rval = std::make_pair(1, MakeOkIPC(1));
                break;
        }
        default: {
            error:
                rval = std::make_pair(1, MakeFailIPC(1));
                break;
        }
    }
    return rval;
}
