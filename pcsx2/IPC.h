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

/* Internal function, thread used to relay IPC commands. */
void SocketThread();

/* Internal function, Parses an IPC command.
 * buf: buffer containing the IPC command.
 * return value: buffer containing the result of the command. */
char* ParseCommand(char* buf);

    public: 

/* Initializers */
SocketIPC();
~SocketIPC();

/* Starts the event-based socket thread. Does nothing if already started. */
void Start();

/* Stops the event-based socket thread. Does nothing if already stopped. */
void Stop();


}; // class SocketIPC
