class SocketIPC {

    private:
void m_thread = NULL;
const char* SOCKET_NAME = "/tmp/pcsx2";

/* Internal function, thread used to relay IPC commands. */
void SocketThread();

/* Internal function, Parses an IPC command.
 * buf: buffer containing the IPC command.
 * return value: buffer containing the result of the command. */
char* ParseCommand(char* buf);

    public: 

/* Initializers */
void SocketIPC();
void ~SocketIPC();

/* Starts the event-based socket thread. Does nothing if already started. */
void Start();

/* Stops the event-based socket thread. Does nothing if already stopped. */
void Stop();


} // class SocketIPC
