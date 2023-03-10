#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
	
int main() {
    char path[] = "server.c";								// Путь к серверу.
    key_t key;
    if ((key = ftok(path, 0)) < 0) {							// Генерируем ключ.
	printf("No server file to create a key!\n");
	exit(-1);
    }
	
    int shm;
    if ((shm = shmget(key, 2000*sizeof(char), 0666|IPC_CREAT)) < 0) {		        // Подключаемся к разделяемой памяти.
	printf("Can't get an access to memory!\n");
	exit(-1);
    }
	
    char *buff;
    if ((buff = (char *)shmat(shm, NULL, 0)) == (char *)(-1)) {				// Записываем в буфер элементы из разделяемой памяти.
	printf("Can't use memory!\n");
    }
	
    int curr = 0;
    while(buff[curr] != '\0') {
	printf("%d ", buff[curr]);						        // Печатаем элементы массива.
	++curr;
    }
	
    if (shmdt(buff) < 0) {
	printf("Can't disconnect memory!\n");						// Удаляем сервер.
	exit(-1);
    }
	
    if (shmctl(shm, 0, NULL) < 0) {
	printf("Can't free memory!\n");							// Освобождаем память.
	exit(-1);
    }
	
    return 0;
}
