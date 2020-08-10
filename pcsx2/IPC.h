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

/* Client code example for interfacing with the IPC interface is available 
 * here: https://code.govanify.com/govanify/pcsx2_ipc/ */

#pragma once

/* Formatting utilities to get a uint from a char* */
#define to64b(arr) (((uint64_t)(((uint8_t*)(arr))[7]) << 0) +  \
        ((uint64_t)(((uint8_t*)(arr))[6]) << 8) +  \
        ((uint64_t)(((uint8_t*)(arr))[5]) << 16) + \
        ((uint64_t)(((uint8_t*)(arr))[4]) << 24) + \
        ((uint64_t)(((uint8_t*)(arr))[3]) << 32) + \
        ((uint64_t)(((uint8_t*)(arr))[2]) << 40) + \
        ((uint64_t)(((uint8_t*)(arr))[1]) << 48) + \
        ((uint64_t)(((uint8_t*)(arr))[0]) << 56))

#define to32b(arr) (((uint32_t)(((uint8_t*)(arr))[3]) << 0) +  \
        ((uint32_t)(((uint8_t*)(arr))[2]) << 8) +  \
        ((uint32_t)(((uint8_t*)(arr))[1]) << 16) + \
        ((uint32_t)(((uint8_t*)(arr))[0]) << 24))

#define to16b(arr) (((uint16_t)(((uint8_t*)(arr))[1]) << 0) + \
        ((uint16_t)(((uint8_t*)(arr))[0]) << 8))

#define to8b(arr) (((uint8_t)(((uint8_t*)(arr))[0]) << 0))

#include "Utilities/PersistentThread.h"
#include "System/SysThreads.h"

using namespace Threading;

class SocketIPC : public pxThread {

	typedef pxThread _parent;

    protected:

#ifdef _WIN32
        // windows claim to have support for AF_UNIX sockets but that is a blatant lie, 
        //their SDK won't even run their own examples, so we go on TCP sockets.
#define PORT 28011
#else
        // absolute path of the socket. Stored in the temporary directory in linux since
        // /run requires superuser permission
        const char* SOCKET_NAME = "/tmp/pcsx2.sock";
#endif

        // socket handlers
#ifdef _WIN32
        SOCKET m_sock = INVALID_SOCKET;
#else
        int m_sock = 0;
#endif

        // possible command messages
        enum IPCCommand {
            MsgRead8 = 0,
            MsgRead16 = 1,
            MsgRead32 = 2,
            MsgRead64 = 3,
            MsgWrite8 = 4,
            MsgWrite16 = 5,
            MsgWrite32 = 6,
            MsgWrite64 = 7
        };

        // possible result codes
        enum IPCResult {
            IPC_OK = 0,
            IPC_FAIL = 0xFF
        };

        // handle to the main vm thread
        SysCoreThread * m_vm;

        /* Thread used to relay IPC commands. */
        void ExecuteTaskInThread();

        /* Internal function, Parses an IPC command.
         * buf: buffer containing the IPC command.
         * return value: pair containing a buffer with the result 
         *               of the command and its size. */
        std::pair<int, char*> ParseCommand(char* buf);

        /* Formats an IPC buffer
         * size: size of the array to allocate
         * return value: buffer containing the status code allocated of size
         *               size */
        static char* MakeOkIPC(int size);
        static char* MakeFailIPC(int size);

        /* Converts an uint to an char* in little endian 
         * res_array: the array to modify 
         * res: the value to convert
         * i: when to insert it into the array 
         * return value: res_array */
        static char* from64b(char* res_array, uint64_t res, int i);
        static char* from32b(char* res_array, uint32_t res, int i);
        static char* from16b(char* res_array, uint16_t res, int i);
        static char* from8b(char* res_array, uint8_t res, int i);

    public:
        /* Initializers */
        SocketIPC(SysCoreThread *vm);
        virtual ~SocketIPC();

}; // class SocketIPC
