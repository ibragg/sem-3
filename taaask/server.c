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

void sigint(){
    printf("\n ^C caught!!!\n");
    if (remove (ServerFIFO) == -1){
        printf("Error\n");
        exit(-1);
    }
    exit(0);
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


    umask(0);
    if (mkfifo(ServerFIFO, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo");
        exit(-1);
    }

    serverFd = OpenFD(ServerFIFO, O_RDWR);

    signal(SIGINT, sigint);

    for (;;) {

        char ClientFIFO[64];
        struct Request request;

        if (read(serverFd, &request, sizeof(struct Request)) < 0) {
            perror("Some requests are incorrect");
            continue;
        }

        sprintf(ClientFIFO, "%d.fifo", request.pid);

        if ((clientFd = open(ClientFIFO, O_RDONLY)) < 0) {
            perror("Could not contact with some clients");
            continue;
        }

        char buf[PAGE_SIZE] = "";
        int RD;

        for (;;) {

            if ((RD = read(clientFd, buf, PAGE_SIZE)) < 0) {
                perror("Client could not read from FIFO");
                exit(-1);
            };
            if (!RD) break;
            if ((write(1, buf, RD)) < 0) {
                perror("Client could not write correctly");
                exit(-1);
            };

        }


        close(clientFd);


    }
    close(serverFd);
}
