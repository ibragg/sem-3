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

char clientfifo[CLIENT_FIFO_NAME_LEN];


int main(int argc, char *argv[]) {


    struct request req;


    if (argc != 2) {
        printf("invalid argc\n");
        exit(1);
    }


    /*  */

    umask(0);                   /* Делаем фифошку клиента*/
    snprintf(clientfifo, CLIENT_FIFO_NAME_LEN, CLIENT_FIFO_TEMPLATE, (long) getpid());
    if (mkfifo(clientfifo, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo");
        exit(1);
    }



    /* создаем запрос, открываем трубу сервера */

    req.pid = getpid();
    strcpy(req.filename, argv[1]);

    int serverFd = Openfd(SERVER_FIFO, O_WRONLY | O_NONBLOCK, "");
    DisableNONBLOCK(serverFd);



    /* открываем нашу трубу, если открывать без нонблока то она будет ждать пока откроется второй конец, нам это не нужно */
    int clientFd = Openfd(clientfifo, O_RDONLY | O_NONBLOCK, "file receiverFd open");
    DisableNONBLOCK(clientFd);  /* выключаем нонблок и переходим в блокирующий режим */

    /* после того как все сделали пише серверу запрос */
    write(serverFd, &req, sizeof(struct request));

    char buf[PIPE_BUF] = "";
    int indicator = 0;
    int reallength = PIPE_BUF;

    /* внутренний цикл нужен для того чтобы ждать ответа сервера только определенное время */

    while (reallength == PIPE_BUF &&
           !buf[0]) {   /* первый байт каждого сообщения это 0 если оно не последнее и 1 если последнее */
        int i = 0;
        while (i < 5)  /*  ждем до 5 секунд*/
        {
            ioctl(clientFd, FIONREAD, &indicator);  /* проверить если ли инфа в трубе */
            if (indicator) {
                break;
            }
            i++;
            sleep(1);
        }

        if (i == 5) {     /* если 5 секунд прошли то ошибка */
            printf("server failed or going too slow\n");
            exit(1);
        }
        reallength = read(clientFd, buf, PIPE_BUF);
        write(STDOUT_FILENO, buf + 1, reallength - 1);

    }

    Closefd(clientFd, "rewriteFd close");
    Closefd(serverFd, "contactFd close");

}