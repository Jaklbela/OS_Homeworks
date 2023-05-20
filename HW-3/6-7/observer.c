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
    
    struct sockaddr_in server_socket;
    int observer_socket;                    // Сокет наблюдателя.
    if ((observer_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Failed to cretae a socket")
        exit(-1);
    }
    server_socket.sin_family = AF_INET;         // Настраиваемся на сервер.
    server_socket.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &server_socket.sin_addr);
    sleep(2);
                                // Подключаемся к серверу.
    if (connect(observer_socket, (struct sockaddr *) &server_socket, sizeof(server_socket)) < 0) {
        printf("Failed to connect to server");
        exit(-1);
    }
    
    char message[4000];
    while (true) {                  // Получаем данные с сервера.
        int received_bytes = recv(observer_socket, update, sizeof(message) - 1, 0);
        if (received_bytes <= 0) {
            break;
        }
        message[received_bytes] = '\0';
        printf("%s", message);
    }
    close(observer_socket);
    return 0;
}
