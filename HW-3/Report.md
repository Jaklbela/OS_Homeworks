# Жмурина Ксения Игоревна БПИ218, ИДЗ-3,  вариант 34
## Узнав о планах преступников по шифрованию текстов, Шерлок Холмс предложил лондонской полиции специальную машину для дешифровки сообщений злоумышленников. Реализовать приложение, дешифрующее кодированный текст. В качестве ключа используется известная кодовая таблица, устанавливающая однозначное соответствие между каждой буквой и каким-нибудь числом. Процессом узнавания кода в решении задачи пренебречь. Каждый процесс–дешифровщик декодирует свои кусочки текста, многократно получаемые от менеджера. Распределение фрагментов текста между процессами–дешифровщиками осуществляется процессом–менеджером, который передает каждому процессу фрагмент зашифрованного текста, получает от него результат, передает следующий зашифрованный фрагмент. Он же собирает из отдельных фрагментов зашифрованный текст. Количество процессов задается опционально. Каждый процесс может выполнять свою работу за случайное время.Сервер — процесс–менеджер. Клиенты — процессы–дешифровщики.
## Клиент принимает на вход IP-адрес и порт сервера, после чего связывается с ним. Получая от сервера кусочки текста, клиент дешифрует их и отправляет обратно. 
## Оценка 4-5. Клиенты отображают информацию независимо..
### Код клиента на си:
```c
#include <stdio.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>

#define MAX_SIZE 4000

int codes[27] = {5, 11, 8, 10, 3, 16, 29, 31, 4, 9, 17,		// Система кодов.
                 38, 42, 54, 6, 12, 28, 39, 36, 7, 14,
                 19, 22, 27, 34, 2, 16};

char decode(int code) {			// Декодер.
    for (int i = 0; i < 27; ++i) {
	if (code == 16) {
	    return ' ';
        } else if (codes[i] == code) {
            return 'a' + i;
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("Wrong arguments");
        exit(-1);
    }
    
    int socket_d;
    int received_bytes;
    int buffer[MAX_SIZE];
    struct sockaddr_in serv_addr;               // Адрес сервера.
    unsigned short serv_port = atoi(argv[3]);   // Порт сервера.
    char *serv_ip = argv[2];                    // IP сервера.

    if ((socket_d = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {   // Создаем TCP-сокет.
        printf("Socket error");
        exit(-1);
    }
            
    memset(&serv_addr, 0, sizeof(serv_addr));       // Настраиваемся на сервер.
    serv_addr.sin_family      = AF_INET; 
    serv_addr.sin_addr.s_addr = inet_addr(serv_ip); 
    serv_addr.sin_port        = htons(serv_port);

            // Устанваливаем соединение с сервером.
    if (connect(socket_d, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection failed");
        exit(-1);
    }
            
            // Считываем данные в буфер с сервера.
    if ((received_bytes = recv(socket_d, buffer, sizeof(buffer), 0)) < 0) {
        perror("Receiving failed");
        exit(-1);
    }

    for(; received_bytes > 0;) {
        char decoded[MAX_SIZE + 1];
        int i = 0;
        for (; i < received_bytes / 5; ++i) {
            printf("Message recieved: ")
            printf("%d ", buffer[i]);
            decoded[i] = decode(buffer[i]);
        }
        printf("\n");
        decoded[i] = '\0';
                // Отправляем данные на сервер.
        if (send(socket_d, decoded, strlen(decoded), 0) != strlen(decoded)) {
            perror("Unexpected quantity of bytes");
            exit(-1);
        }
        sleep(2);
        if ((received_bytes = recv(socket_d, buffer, sizeof(buffer), 0)) < 0) {
            perror("Receiving failed");
            exit(-1);
        }
    }

    close(socket_d);
    exit(0);
}
```

### Код сервера на си
```c
```
## Оценка 6-7. 
