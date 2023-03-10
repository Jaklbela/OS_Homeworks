#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
	
int main() {
    int arr[15];								// Создаем массив со случайными числами.
    srand(time(0));
    for (int i = 0; i < 15; i++) {
        arr[i] = rand() % 100;							// Генерируем числа от 0 до 100.
    }
    
    char path[] = "server.c";							// Путь до сервера.
    key_t key;
    if ((key = ftok(path,0)) < 0) {						// Генерируем ключ.
	printf("No server file to create a key!\n");
	exit(-1);
    }
	
    int shm;
    if ((shm = shmget(key, 2000*sizeof(char), 0666|IPC_CREAT)) < 0) {	        // Создаем разделяемую память.
        printf("Can't create memory!\n");
        exit(-1);
    }
	
    char *buff;
    if ((buff = (char *)shmat(shm, NULL, 0)) == (char *)(-1)) {		        // Создаем буфер.
        printf("Can't use memory!\n");
        exit(-1);
    }
	
    int curr = 0;
    while (curr < 15) {
	buff[curr] = arr[curr];						        // Записываем элементы массива в буфер.
	++curr;
    }
	
    if (shmdt(buff) < 0) {
	printf("Can't disconnect memory!\n");				       // Отключаем клиент.
	exit(-1);
    }
    
    return 0;
}
