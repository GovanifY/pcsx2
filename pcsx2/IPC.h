#define to64b(arr) (((uint64_t)(((uint8_t *)(arr))[7]) <<  0)+\
                    ((uint64_t)(((uint8_t *)(arr))[6]) <<  8)+\
                    ((uint64_t)(((uint8_t *)(arr))[5]) << 16)+\
                    ((uint64_t)(((uint8_t *)(arr))[4]) << 24)+\
                    ((uint64_t)(((uint8_t *)(arr))[3]) << 32)+\
                    ((uint64_t)(((uint8_t *)(arr))[2]) << 40)+\
                    ((uint64_t)(((uint8_t *)(arr))[1]) << 48)+\
                    ((uint64_t)(((uint8_t *)(arr))[0]) << 56))

#define to32b(arr) (((uint32_t)(((uint8_t *)(arr))[3]) <<  0)+\
                    ((uint32_t)(((uint8_t *)(arr))[2]) <<  8)+\
                    ((uint32_t)(((uint8_t *)(arr))[1]) << 16)+\
                    ((uint32_t)(((uint8_t *)(arr))[0]) << 24))

#define to16b(arr) (((uint16_t)(((uint8_t *)(arr))[1]) <<  0)+\
                    ((uint16_t)(((uint8_t *)(arr))[0]) <<  8))

#define to8b(arr) (((uint16_t)(((uint8_t *)(arr))[0]) <<  0))



class SocketIPC {

    private:

// absolute path of the socket. Stored in the temporary directory in linux since
// /run requires superuser permission
const char* SOCKET_NAME = "/tmp/pcsx2";

// currently running thread identifier
int m_thread = 0;

// socket handlers
int m_sock, m_msgsock = 0;


// possible command names
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

// possible states of the IPC
enum State {
    Started,
    Stopped
};

// current state of the IPC
State m_state = Stopped;

/* Internal function, thread used to relay IPC commands. */
void SocketThread();

/* Internal function, Parses an IPC command.
 * buf: buffer containing the IPC command.
 * return value: buffer containing the result of the command. */
std::tuple<int, char*> ParseCommand(char* buf);

    public: 

/* Initializers */
SocketIPC();
~SocketIPC();

/* Starts the event-based socket thread. Does nothing if already started. */
void Start();

/* Stops the event-based socket thread. Does nothing if already stopped. */
void Stop();


}; // class SocketIPC
