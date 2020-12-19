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

void CloseFD(int fd) {
    if (close(fd) == -1) {
        perror("Client could not close the file");
        exit(-1);
    }
}

int OpenFD(const char *name, int flags) {

    int fd;
    if ((fd = open(name, flags)) < 0) {
        printf("Client could not open %s \n", name);
        exit(-1);
    }
    return fd;
}


int main(int argc, char *argv[]) {

    if (argc != 2) {
        perror("Arguments are incorrect \n");
        exit(-1);
    }

    struct Request request;
    request.pid = getpid();
    strcpy(request.filename, argv[1]);


    umask(0);
    char ClientFIFO[64];
    sprintf(ClientFIFO, "%d.fifo", getpid());

    if (mkfifo(ClientFIFO, 0666) == -1 && errno != EEXIST) {
        perror("Server could not make FIFO");
        exit(-1);
    }


    int serverFd = OpenFD(ServerFIFO, O_WRONLY);

    write(serverFd, &request, sizeof(struct Request));

    int clientFd = OpenFD(ClientFIFO, O_WRONLY);

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
    close(clientfile);
    CloseFD(clientFd);
    CloseFD(serverFd);
    remove(ClientFIFO);

}
