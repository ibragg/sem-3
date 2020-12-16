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

#define ServerFIFO "FILE.FIFO"

#define PAGE_SIZE 4096

struct Request {
    pid_t pid;
    char filename[256];

};

void sigint(int a){
    printf("\n ^C caught!!!\n");
}

void CloseFD(int fd) {
    if (close(fd) == -1) {
        perror("Server could not close the file");
        exit(-1);
    }
}

int OpenFD(const char *name, int flags) {

    int fd;
    if ((fd = open(name, flags)) < 0) {
        printf("Server could not open %s\n", name);
        exit(-1);
    }
    return fd;
}


int main() {
    int serverFd, clientFd;
    signal(SIGINT, sigint);

    umask(0);
    if (mkfifo(ServerFIFO, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo");
        exit(-1);
    }

    serverFd = OpenFD(ServerFIFO, O_RDWR);

    signal(SIGPIPE, SIG_IGN);

    for (;;) {

        char ClientFIFO[64];
        struct Request request;

        if (read(serverFd, &request, sizeof(struct Request)) < 0) {
            perror("Some requests are incorrect");
            continue;
        }

        sprintf(ClientFIFO, "%d.fifo", request.pid);

        if ((clientFd = open(ClientFIFO, O_WRONLY)) < 0) {
            perror("Could not contact with some clients");
            continue;
        }


        int clientfile;

        if ((clientfile = open(request.filename, O_RDONLY)) < 0) {
            perror("Could not open the file.");
            continue;
        }

        char buf[PAGE_SIZE] = "";
        int RD;

        while (1) {

            if ((RD = read(clientfile, buf, PAGE_SIZE)) < 0) {
                perror("Could not read read the file.");
            }

            if (!RD) break;

            if (write(clientFd, buf, RD) < 0) {
                perror("Could not send to the client content of the file.");
                break;
            }

        }

        close(clientFd);
        close(clientfile);

    }
    close(serverFd);
}

