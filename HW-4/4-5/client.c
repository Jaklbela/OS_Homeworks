#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#define MAX_SIZE 4000

int codes[27] = {5, 11, 8, 10, 3, 16, 29, 31, 4, 9, 17,    // Система кодов.
                 38, 42, 54, 6, 12, 28, 39, 36, 7, 14,
                 19, 22, 27, 34, 2, 16};

char decode(int code) {      // Декодер.
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


    if ((socket_d = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {  // Создаем UDP-сокет.
        printf("Socket error");
        exit(-1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));       // Настраиваемся на сервер.
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(serv_ip);
    serv_addr.sin_port        = htons(serv_port);

    char *st = "Connected";
    if (sendto(socket_d, st, strlen(st), 0, (struct sockaddr *)
            &serv_addr, sizeof(serv_addr)) != strlen(st)) {
        printf("Sent a different number of bytes than expected.");
        exit(-1);
    }

    struct sockaddr_in inp_addr;     /* Source address of echo */
    unsigned int inp_size = sizeof(inp_addr);

    if ((received_bytes = recvfrom(socket_d, buffer, sizeof(buffer), 0,
                                   (struct sockaddr *) &inp_addr, &inp_size)) < 0) {
        printf("Receiving failed");
        exit(-1);
    }
    if (serv_addr.sin_addr.s_addr != inp_addr.sin_addr.s_addr) {
        printf("Received a packet from unknown source.");
        exit(-1);
    }

    char decoded[MAX_SIZE + 1];
    for(; received_bytes > 0;) {
        int i = 0;

        if (received_bytes == 5 && buffer[0] == -1) {
            break;
        }

        for (; i < received_bytes / 5; ++i) {
            printf("Message recieved: ");
            printf("%d ", buffer[i]);
            decoded[i] = decode(buffer[i]);
        }
        printf("\n");
        decoded[i] = '\0';

        if (sendto(socket_d, decoded, strlen(decoded), 0, (struct sockaddr *)
                &serv_addr, sizeof(serv_addr)) != strlen(decoded)) {
            perror("Unexpected quantity of bytes");
            exit(-1);
        }
        sleep(2);
        if ((received_bytes = recvfrom(socket_d, buffer, sizeof(buffer), 0,
                                       (struct sockaddr *) &inp_addr, &inp_size)) < 0) {
            perror("Receiving failed");
            exit(-1);
        }
        if (serv_addr.sin_addr.s_addr != inp_addr.sin_addr.s_addr) {
            printf("Received a packet from unknown source.");
            exit(-1);
        }
    }

    close(socket_d);
    exit(0);
}
