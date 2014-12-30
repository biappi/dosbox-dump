#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <sys/wait.h> 
#include <fcntl.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <stdarg.h>

static const int NETBUGGER_PORT = 6969;

static FILE *pd                 = NULL;

static bool netbugger_inited    = false;
static int  netbugger_socket_fd = 0;

// BAD GLOBALS ARE BAD --v

struct {
    bool            want_response;
    char            command[1024];
    int             client_fd;
    bool            capturing;

    pthread_mutex_t client_connection_lock;
    pthread_mutex_t command_string_lock;
    pthread_cond_t  command_completion_cond;
} netbugger_command;

// BAD GLOBALS ARE BAD --^

void n_log(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(pd, fmt, args);
    va_end(args);
    fflush(pd);
}

void NETBUG_PutCommand(char * command);

static
int incoming_command(int fd, char * command)
{
    n_log("incoming: >%s< (%d)\n", command, strlen(command));

    if (pthread_mutex_lock(&netbugger_command.client_connection_lock) != 0) {
        n_log("error connection lock\n");
        return -1;
    }

    netbugger_command.client_fd= fd;
    NETBUG_PutCommand(command);

    if (pthread_cond_wait(&netbugger_command.command_completion_cond,
                          &netbugger_command.client_connection_lock) != 0)
    {
        n_log("error command wait\n");
        return -1;
    }

    if (pthread_mutex_unlock(&netbugger_command.client_connection_lock) != 0) {
        n_log("error connection unlock\n");
        return -1;
    }

    return 0;
}

static
void connection_handler(int fd)
{
    const char * s = "succhia\n";
    if (send(fd, s, strlen(s), 0) == -1) {
        perror("send");
        return;
    }

    char   read    [1024];
    char   command [1024];

    char * command_end = command + sizeof(command);
    char * command_top = command;

    while (1) {
        int  n = recv(fd, read, sizeof(read), 0);
        if (n == 0) {
            n_log("Bye bye\b");
            return;
        }
        if (n == -1) {
            perror("recv");
            return;
        }

        for (int i = 0; i < n; i++) {
            if (read[i] == '\n') {
                *command_top = 0;

                if (incoming_command(fd, command) == -1)
                    return;

                memset(command, 0, sizeof(command));
                command_top = command;
            }
            else if (read[i] == '\r') {
                // skip it so we can telnet to it
                command_top++;
            }
            else if (!isprint(read[i])) {
                const char * e = "E: Printable chars, please\n";
                send(fd, e, strlen(e), 0);
                return;
            }
            else {
                *command_top = read[i];
                command_top++;
            }

            if (command_top >= command_end) {
                const char * e = "E: Command too long\n";
                if (send(fd, e, strlen(e), 0) == -1)
                    return;

                memset(command, 0, sizeof(command));
                command_top = command;
                break;
            }
        }
    }
}

static
void * listening_thread(void * unused)
{
    while (1) {
        n_log("## BEGIN LISTEN\n");
        if (listen(netbugger_socket_fd, 10) == -1) {
            perror("listen");
            return NULL;
        }

        n_log("## BEGIN ACCEPT\n");
        sockaddr_in client_addr;
        socklen_t   client_addr_size;
        int client_socket = accept(netbugger_socket_fd,
                                   (struct sockaddr *)&client_addr,
                                   &client_addr_size);
        if (client_socket == -1) {
            perror("accept");
            return NULL;
        }

        n_log("## ACCEPTED\n");
        connection_handler(client_socket);
        n_log("## CONNECTION ENDED\n");

        if (close(client_socket) == -1) {
            perror("close");
        }

        n_log("## CONNECTION CLOSED\n");
    }
}

static
int NETBUG_Init()
{
    if (pthread_mutex_init(&netbugger_command.client_connection_lock, NULL) != 0) {
        n_log("error client mutex init\n");
        return -1;
    }

    if (pthread_mutex_init(&netbugger_command.command_string_lock, NULL) != 0) {
        n_log("error command init\n");
        return -1;
    }

    if (pthread_cond_init(&netbugger_command.command_completion_cond, NULL) != 0) {
        n_log("error condition init\n");
        return -1;
    }

    return 0;    
}

void NETBUG_PutCommand(char * command)
{
    if (pthread_mutex_lock(&netbugger_command.command_string_lock) != 0) {
        n_log("error mutex_lock\n");
        return;
    }

    netbugger_command.want_response = true;
    strncpy(netbugger_command.command, command, sizeof(netbugger_command.command));

    if (strlen(netbugger_command.command))
        n_log("put:  %s\n", netbugger_command.command);

    if (pthread_mutex_unlock(&netbugger_command.command_string_lock) != 0) {
        n_log("error mutex_unlock\n");
        return;
    }
}

bool NETBUG_WantResponse() {
    return netbugger_command.want_response;
}

int NETBUG_GetCommand(char * buffer, size_t len)
{
    if (pthread_mutex_trylock(&netbugger_command.command_string_lock) != 0)
        return -1;

    strncpy(buffer, netbugger_command.command, len);

    if (netbugger_command.command[0]) {
        netbugger_command.command[0] = 0;
        n_log("got %s\n", buffer);
    }

    if (pthread_mutex_unlock(&netbugger_command.command_string_lock) != 0) {
        n_log("error mutex unlock\n");
        return -1;
    }

    return 0;
}

void NETBUG_BeginCaptureOutput()
{
    netbugger_command.capturing = true;
}

void NETBUG_EndCaptureOutput()
{
    netbugger_command.capturing = false;
}

void NETBUG_FinishCommand(bool success) {
    const char * ok = "C: OK\n";
    const char * ko = "C: KO\n";

    const char * to_send = success ? ok : ko;

    if (send(netbugger_command.client_fd, to_send, strlen(to_send), 0) == -1) {
        n_log("can't send it\n");
    }

    netbugger_command.client_fd = 0;
    netbugger_command.want_response = false;
 
    if (pthread_cond_signal(&netbugger_command.command_completion_cond) != 0) {
        n_log("can't signal the command's over\n");
    }
}

void NETBUG_SendMsg(char * buf)
{
    if (!netbugger_command.capturing || netbugger_command.client_fd == 0)
        return;

     // buf from debug_gui.cpp is hardcoded to 512
    char to_send[1024] = {0};
    char *buf_i  = buf;
    char *send_i = to_send;
    bool newline = true;

    while (*buf_i != 0) {
        if (newline) {
            *send_i++ = 'O';
            *send_i++ = ':';
            *send_i++ = ' ';
        }

        *send_i = *buf_i;
        newline = (*buf_i == '\n');

        buf_i++;
        send_i++;
    } 

    if (send(netbugger_command.client_fd, to_send, send_i - to_send, 0) == -1) {
        n_log("can't send it\n");
    }
}


void NETBUG_StartServer()
{
    pd = fopen("/tmp/porcoddio", "w+");

    if (NETBUG_Init() != 0) {
        n_log("error netbug init\n");
        return;
    }

    if ((netbugger_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        n_log("socket error\n");
        return;
    }

    int reuse = 1;
    setsockopt(netbugger_socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));

    struct sockaddr_in addr = {0};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(NETBUGGER_PORT);

    if (bind(netbugger_socket_fd,
             (const struct sockaddr *)&addr,
             sizeof(addr)) == -1)
    {
        perror("bind");
        return;
    }

    pthread_t listening_pthread;
    if (pthread_create(&listening_pthread, NULL,
                       listening_thread, NULL) != 0)
    {
        n_log("error creating listening thread\n");
        return;
    }

    n_log("STARTED\n");
}

