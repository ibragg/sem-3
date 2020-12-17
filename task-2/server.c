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


int Sem(int semId, int semNum, int initVal) {
    return semctl(semId, semNum, SETVAL, initVal);
}

void File_Receiver(int semid, void *addr) {

    struct sembuf sops[4];

    INIT(0, 1, 0, 0)
    INIT(1, 1, 1, SEM_UNDO)
    semop(semid, sops, 2);

    INIT(0, 4, 0, 0);
    semop(semid, sops, 1);

    Sem(semid, 2, 2);

    INIT(0, 1, 1, SEM_UNDO)
    INIT(1, 2, -1, SEM_UNDO)
    semop(semid, sops, 2);

    INIT(0, 0, -2, 0)
    INIT(1, 0, 2, 0)
    INIT(2, 4, 1, SEM_UNDO)
    semop(semid, sops, 3);

    int indicator = 4092;

    while (indicator == 4092) {
        SEM(3, -1, 0)
        indicator = *(int *) addr;
        write(STDOUT_FILENO, addr + 4, indicator);
        SEM(2, 1, 0)
    }
}

int main(int argc, char *argv[]) {
    int semid = semget(ftok("server.c", 10), 5, IPC_CREAT | 0666);
    int shmid = shmget(ftok("server.c", 10), 4096, IPC_CREAT | 0666);
    void *addr = shmat(shmid, NULL, 0);

    File_Receiver(semid, addr);
    return 0;
}
