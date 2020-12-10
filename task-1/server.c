#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>

#define SERVER_FIFO "/tmp/seqnum_sv"
/* Well-known name for server's FIFO */
#define CLIENT_FIFO_TEMPLATE "/tmp/seqnum_cl.%ld"
/* Template for building client FIFO name */
#define CLIENT_FIFO_NAME_LEN (sizeof(CLIENT_FIFO_TEMPLATE) + 20)
/* Space required for client FIFO pathname
   (+20 as a generous allowance for the PID) */

struct request {                /* Request (client --> server) */
    pid_t pid;                  /* PID of client */
    char filename[256];
};

const int SIZEOFCHAR = sizeof(char);
const int SIZEOFPID_T = sizeof(pid_t);

void Closefd(int fd, const char *msg) {
    if (close(fd) == -1) {
        perror(msg);
        exit(1);
    }
}

int Openfd(const char *name, int flags, const char *msg) {

    int Fd = open(name, flags);
    if (Fd == -1) {
        perror(msg);
        exit(1);
    }
    return Fd;
}

void DisableNONBLOCK(int fd) {
    int flags;

    flags = fcntl(fd, F_GETFL);
    flags &= ~O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);

}

int main(int argc, char *argv[])
{
    int serverFd, clientFd;
    char clientfifo[CLIENT_FIFO_NAME_LEN];
    struct request req;


    /* открываем трубу сервера (если клиент откроет ее раньше нас то умрет и это логично)*/

    umask(0);
    if (mkfifo(SERVER_FIFO, 0666) == -1 && errno != EEXIST){
        perror("mkfifo");
        exit(1);
    }

    serverFd = Openfd(SERVER_FIFO, O_RDWR, "");

    signal(SIGPIPE, SIG_IGN);  /* если словим сигпайп от записи в трубу без кнца на чтение(потому что клиент умер)
                                сервер не умрет и это важно */



    for (;;) {                          /* цикл обработки запросов, на любую ошибку просто идем дальше*/
        if (read(serverFd, &req, sizeof(struct request)) != sizeof(struct request)) {
            fprintf(stderr, "Error reading request\n");
            continue;
        }

        /* после того как считали структуру, открываем клиентское фифо и файл, который нам передали */

        snprintf(clientfifo, CLIENT_FIFO_NAME_LEN, CLIENT_FIFO_TEMPLATE, (long) req.pid);
        int clientFd = open(clientfifo, O_WRONLY | O_NONBLOCK);
        if (clientFd == -1) {
            perror("CLIENT");
            continue;
        }
        DisableNONBLOCK(clientFd);

        FILE* flin = fopen(req.filename, "rb");
        if (flin == NULL) {
            perror("CLIENT");
            continue;
        }

        char buf[PIPE_BUF] = "";
        int reallength = 0;


        /* первый байт - индикатор сообщения, пишем пока reallength работает как надо */


        while((reallength = fread(buf + 1, SIZEOFCHAR,  PIPE_BUF - 1, flin)) == PIPE_BUF - 1 ){


            buf[0] = 0;
            if(write(clientFd, buf, PIPE_BUF) == -1){
                perror("CLIENT");
                close(clientFd);
                break;
            }


        }
        buf[0] = 1;
        write(clientFd, buf, reallength + 1);


        close(clientFd);           /* закрываем клиентскую трубу, идем к следующему клиенту */
    }
}

