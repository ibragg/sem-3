#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>



#define opINIT(n, sem, op, flg) sops[n].sem_num = sem;					\
						    	sops[n].sem_op = op;					\
						    	sops[n].sem_flg = flg;


#define opSEM(sem, op, flg)		opINIT(0, sem, op, flg)					\
	    						semop(semid, sops, 1);



#define PAGE_SIZE 4096
const char SIZEOFINT = 4;
const char SIZEOFCHAR = 1;
#define NAMETOFTOK "server.c"



int initSem(int semId, int semNum, int initVal){
    return semctl(semId, semNum, SETVAL, initVal);
}

void File_Translator(char* filename, int semid, void* addr){

    FILE* flin = fopen(filename, "rb");

    struct sembuf sops[4];


    opINIT(0, 0, 0, 0) /*  берем флаг клиентов чтобы ни один клиент не мог больше зайти и сломать нам все
		так как непонятно могут ли запускаться другие*/
    opINIT(1, 0, 1, SEM_UNDO)
    semop(semid, sops, 2);


    opINIT(0, 4, 0, 0); /*если до нашей пары клиент сервер была запущена еще одна такая пара ждем пока она корректно завершит работу*/
    semop(semid, sops, 1);


    initSem(semid, 3, 1);


    opINIT(0, 0, 1, SEM_UNDO); /*если мы умрем нам обязательно надо вернуть флаг 3ему семафору иначе блокнемся
									так же сообщаем серверу что мы проинициализировали семафор 3 и он может идти дальше*/
    opINIT(1, 3, -1, SEM_UNDO)
    semop(semid, sops, 2);


    opINIT(0, 1, -2, 0) /*ждем пока сервер проинициализирует наш семафор и говорим что мы начали работу прибавляя 1 к 4ому семафору*/
    opINIT(1, 1, 2, 0)
    opINIT(2, 4, 1, SEM_UNDO);
    semop(semid, sops, 3);


    int reallength = PAGE_SIZE - SIZEOFINT;

    while(reallength == PAGE_SIZE - SIZEOFINT){

        /*процесс печати, берем флаг на печать(который выставил сервер) пишем и поднимаем флаг на чтение
            ждем пока нам поднимут флаг на запись и по новой*/
        opSEM(2, -1, 0)

        if(semctl(semid, 4, GETVAL) != 2){ //проверяем живой ли второй процесс, если нет то нам надо умереть, попасть сюда можем так как он при смерти выставил нужный семафор

            break;
        }

        reallength = fread(addr + SIZEOFINT, SIZEOFCHAR,  PAGE_SIZE - SIZEOFINT, flin);
        *(int*)addr = reallength;

        opSEM(3, 1, 0)

    }


}



int main(int argc, char* argv[]){
    if (argc > 2 || argc < 1){
        printf("Invalid argc\n");
        return 0;
    }

    int semid = semget(ftok(NAMETOFTOK, 10), 5, IPC_CREAT | 0666);
    if( semid == -1){
        perror("semid");
        exit(1);
    }
    //0-file translator
    //1-file receiver
    //2-shmwrite
    //3-shmread
    //4-synchronyze

    int shmid = shmget(ftok(NAMETOFTOK, 10), PAGE_SIZE, IPC_CREAT | 0666);
    if( shmid == -1){
        perror("shmid");
        exit(1);
    }
    void* addr = shmat(shmid, NULL, 0);


    File_Translator(argv[1], semid, addr);


    return 0;
}
