/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2020  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

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
#include "System/SysThreads.h"
#include "IPC.h"

SocketIPC::SocketIPC(SysCoreThread *vm) : pxThread("IPC_Socket") {
#ifdef _WIN32
        WSADATA wsa;
        SOCKET new_socket;
        struct sockaddr_in server, client;
        int c;


        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            Console.WriteLn(Color_Red, "IPC: Cannot initialize winsock! Shutting down...");
            return;
        }

        if ((m_sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
            Console.WriteLn(Color_Red, "IPC: Cannot open socket! Shutting down...");
            return;
        }

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
        
        // we save a handle of the main vm object
        m_vm = vm;

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
    u32 a = FromArray<u32>(buf, 1);
    std::pair<int, char*> rval;
    switch (opcode) {
        case MsgRead8: {
                u8 res;
                if(m_vm->HasActiveMachine() == true) {
                    res = memRead8(a);
                } else { goto error; }
                rval = std::make_pair(2, ToArray(MakeOkIPC(2), res, 1));
                break;
        }
        case MsgRead16: {
                u16 res;
                if(m_vm->HasActiveMachine() == true) {
                    res = memRead16(a);
                } else { goto error; }
                rval = std::make_pair(3, ToArray(MakeOkIPC(3), res, 1));
                break;
        }
        case MsgRead32: {
                u32 res;
                if(m_vm->HasActiveMachine() == true) {
                    res = memRead32(a);
                } else { goto error; }
                rval = std::make_pair(5, ToArray(MakeOkIPC(5), res, 1));
                break;
        }
        case MsgRead64: {
                u64 res;
                if(m_vm->HasActiveMachine() == true) {
                    memRead64(a, &res);
                } else { goto error; }
                rval = std::make_pair(9, ToArray(MakeOkIPC(9), res, 1));
                break;
        }
        case MsgWrite8: {
                if(m_vm->HasActiveMachine() == true) {
                    memWrite8(a, FromArray<u8>(buf, 5));
                } else { goto error; }
                rval = std::make_pair(1, MakeOkIPC(1));
                break;
        }
        case MsgWrite16: {
                if(m_vm->HasActiveMachine() == true) {
                    memWrite16(a, FromArray<u16>(buf, 5));
                } else { goto error; }
                rval = std::make_pair(1, MakeOkIPC(1));
                break;
        }
        case MsgWrite32: {
                if(m_vm->HasActiveMachine() == true) {
                    memWrite32(a, FromArray<u32>(buf, 5));
                } else { goto error; }
                rval = std::make_pair(1, MakeOkIPC(1));
                break;
        }
        case MsgWrite64: {
                if(m_vm->HasActiveMachine() == true) {
                    memWrite64(a, FromArray<u64>(buf, 5));
                } else { goto error; }
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
