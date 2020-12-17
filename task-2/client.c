#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#define INIT(n, sem, op, flg) sops[n].sem_num = sem;                                  \
                                                        sops[n].sem_op = op;                                    \
                                                        sops[n].sem_flg = flg;


#define SEM(sem, op, flg)             INIT(0, sem, op, flg)                                 \
                                                        semop(semid, sops, 1);


#define PAGE_SIZE 4096
#define NAMETOFTOK "server.c"

int Sem(int semId, int semNum, int initVal) {
    return semctl(semId, semNum, SETVAL, initVal);
}

void File_Translator(char *filename, int semid, void *addr) {

    FILE *flin = fopen(filename, "rb");

    struct sembuf sops[4];

    INIT(0, 0, 0, 0)
    INIT(1, 0, 1, SEM_UNDO)
    semop(semid, sops, 2);

    INIT(0, 4, 0, 0);
    semop(semid, sops, 1);

    Sem(semid, 3, 1);

    INIT(0, 0, 1, SEM_UNDO);
    INIT(1, 3, -1, SEM_UNDO)
    semop(semid, sops, 2);

    INIT(0, 1, -2, 0)
    INIT(1, 1, 2, 0)
    INIT(2, 4, 1, SEM_UNDO);
    semop(semid, sops, 3);

    int reallength = 4092;

    while (reallength == 4092) {
        SEM(2, -1, 0)

        reallength = fread(addr + 4, 1, 4092, flin);
        *(int *) addr = reallength;

        SEM(3, 1, 0)
    }
}

int main(int argc, char *argv[]) {
    int semid = semget(ftok(NAMETOFTOK, 10), 5, IPC_CREAT | 0666);
    int shmid = shmget(ftok(NAMETOFTOK, 10), PAGE_SIZE, IPC_CREAT | 0666);
    void *addr = shmat(shmid, NULL, 0);
    File_Translator(argv[1], semid, addr);
    return 0;
}

