#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Wrong arguments");
        exit(-1);
    }
    struct sockaddr_in server_addr;
    int observer_socket;
    if ((observer_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Ошибка при создании сокета");
        exit(1);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);
    sleep(1);
    if (connect(observer_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("Ошибка соединения");
        exit(1);
    }
    char update[4096];
    while (true) {
        int bytes = recv(observer_socket, update, sizeof(update) - 1, 0);
        if (bytes <= 0) {
            break;
        }
        update[bytes] = '\0';
        printf("%s", update);
    }
    close(observer_socket);
    return 0;
}
