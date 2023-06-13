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

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("Wrong arguments");
        exit(-1);
    }

    int socket_d;
    struct sockaddr_in serv_addr; // Адрес сервера.
    unsigned short serv_port;     // Порт сервера.
    char *serv_ip = argv[1];      // IP-адрес сервера.
    struct sockaddr_in inp_addr;
    unsigned int inp_size = sizeof(inp_addr);
    int received_bytes;
    char buffer[10000];

    if ((socket_d = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {    // Cоздаем UDP-сокет.
        perror("Socket failed");
        exit(1);
    }

    // Подключаемся к серверу.
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(serv_ip);
    serv_addr.sin_port        = htons(serv_port);

    char *st = "Connected observer";
    if (sendto(socket_d, st, strlen(st), 0, (struct sockaddr *)
            &serv_addr, sizeof(serv_addr)) != strlen(st)) {
        printf("Sent a different number of bytes than expected");
        exit(-1);
    }

    if ((received_bytes = recvfrom(socket_d, buffer, sizeof(buffer) - 1, 0,
                                   (struct sockaddr *) &inp_addr, &inp_size)) < 0) {
        printf("Receiving failed");
        exit(-1);
    }

    if (serv_addr.sin_addr.s_addr != inp_addr.sin_addr.s_addr) {
        printf("Received a packet from unknown source.");
        exit(-1);
    }

    for(; received_bytes > 0;) {
        
        if (received_bytes == 1 && buffer[0] == '\0') {
            break;
        }
        
        printf("Received message: ");
        buffer[received_bytes] = '\0';
        printf("%s\n", buffer);
        sleep(1);
        
        if ((received_bytes = recvfrom(socket_d, buffer, sizeof(buffer) - 1, 0,
                                       (struct sockaddr *) &inp_addr, &inp_size)) < 0) {
            printf("Receiving failed");
        }
        
        if (serv_addr.sin_addr.s_addr != inp_addr.sin_addr.s_addr) {
            printf("Received a packet from unknown source.");
            exit(-1);
        }
    }

    close(socket_d);
    exit(0);
}
