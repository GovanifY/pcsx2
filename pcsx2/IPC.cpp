#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include "Common.h"
#include "Memory.h"
#include "IPC.h"

// TODO: when pcsx2 has a running SysCoreThread but no memory layout setup it
// asserts, eg by using Shutdown, fix that

SocketIPC::SocketIPC() {
}

void SocketIPC::Start() {
    if(m_state == Stopped) {
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

        // maximum queue of 100 commands before refusing
        listen(m_sock, 100);

        m_state = Started;
        std::thread m_thread(SocketThread, m_sock);
        m_thread.detach();
    }
}

void SocketIPC::SocketThread(int sock) {
    char buf[1024];
    int msgsock = 0;

    while(true) {
        msgsock = accept(sock, 0, 0);
        if (msgsock == -1) {
            Console.WriteLn( Color_Red, "IPC: Connection to socket broken! Shutting down...\n" );
            return;
        }
        else {
            bzero(buf, sizeof(buf));
            if (read(msgsock, buf, 1024) < 0) {
                Console.WriteLn( Color_Red, "IPC: Cannot receive event! Shutting down...\n" );
                return;
            }
            else {
                auto res = ParseCommand(buf);
                if (write(msgsock, std::get<1>(res), std::get<0>(res)) < 0) {
                    Console.WriteLn( Color_Red, "IPC: Cannot send reply! Shutting down...\n" );
                    return;
                }
            }
        }
    }
}
void SocketIPC::Stop() {
    if(m_state == Started) {
        close(m_sock);
        unlink(SOCKET_NAME);
        m_state = Stopped;
    }
}
SocketIPC::~SocketIPC() {
    Stop();
}

// we might want to make some TMP magic here... 
// nvm, we NEED it, 90% of this function is array initialization
std::tuple<int, char*> SocketIPC::ParseCommand(char *buf) {
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
    u32 a = int((unsigned char)(buf[1]) << 24 |
            (unsigned char)(buf[2]) << 16 |
            (unsigned char)(buf[3]) << 8 |
            (unsigned char)(buf[4]));
    char *res_array;
    std::tuple<int, char*> rval;
    switch (opcode) {
        case MsgRead8: {
            u8 res = memRead8(a);
            res_array = (char*)malloc(2*sizeof(char));
            res_array[0] =  0x00; 
            res_array[1] = (unsigned char) res;
            rval =  std::make_tuple(2,res_array);
            break;
        }
        case MsgRead16: {
            u16 res = memRead16(a);
            res_array = (char*)malloc(3*sizeof(char));
            res_array[0] =  0x00; 
            res_array[1] = (unsigned char) (res >> 8) & 0xff;
            res_array[2] = (unsigned char) res;
            rval = std::make_tuple(3,res_array);
            break;
        }
        case MsgRead32: {
            u32 res = memRead32(a);
            res_array = (char*)malloc(5*sizeof(char));
            res_array[0] =  0x00; 
            res_array[1] = (unsigned char) (res >> 24) & 0xff;
            res_array[2] = (unsigned char) (res >> 16) & 0xff;
            res_array[3] = (unsigned char) (res >> 8) & 0xff;
            res_array[4] = (unsigned char) res;
            rval = std::make_tuple(5,res_array);
            break;
        }
        case MsgRead64: {
            u64 res;
            memRead64(a, &res);
            res_array = (char*)malloc(9*sizeof(char));
            res_array[0] =  0x00; 
            res_array[1] = (unsigned char) (res >> 56) & 0xff;
            res_array[2] = (unsigned char) (res >> 48) & 0xff;
            res_array[3] = (unsigned char) (res >> 40) & 0xff;
            res_array[4] = (unsigned char) (res >> 32) & 0xff;
            res_array[5] = (unsigned char) (res >> 24) & 0xff;
            res_array[6] = (unsigned char) (res >> 16) & 0xff;
            res_array[7] = (unsigned char) (res >> 8) & 0xff;
            res_array[8] = (unsigned char) res;
            rval = std::make_tuple(9,res_array);
            break;
        }
        case MsgWrite8: {
            memWrite8(a, to8b(&buf[5]));
            res_array = (char*)malloc(1*sizeof(char));
            res_array[0] =  0x00; 
            rval = std::make_tuple(1,res_array);
            break;
        }
        case MsgWrite16: {
            memWrite16(a, to16b(&buf[5]));
            res_array = (char*)malloc(1*sizeof(char));
            res_array[0] =  0x00; 
            rval = std::make_tuple(1,res_array);
            break;
        }
        case MsgWrite32: {
            memWrite32(a, to32b(&buf[5]));
            res_array = (char*)malloc(1*sizeof(char));
            res_array[0] =  0x00; 
            rval = std::make_tuple(1,res_array);
            break;
        }
        case MsgWrite64: {
            memWrite64(a, to64b(&buf[5]));
            res_array = (char*)malloc(1*sizeof(char));
            res_array[0] =  0x00; 
            rval = std::make_tuple(1,res_array);
            break;
        }
        default: {
            res_array = (char*)malloc(1*sizeof(char));
            res_array[0] =  0xFF; 
            rval = std::make_tuple(1,res_array);
            break;
        }
    }
    return rval;
}
