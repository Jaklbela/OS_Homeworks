#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 4000
#define SOCKET struct sockaddr

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Wrong arguments");
    exit(-1);
  }

  char buffer[BUFFER_SIZE];

  int client_fd = socket(AF_INET, SOCK_STREAM, 0);	// Создаем TCP сокет.
  if (client_fd < 0) {
    perror("Failed to create a TCP socket");
    exit(-1);
  }

  int port = atoi(argv[2]);

  struct sockaddr_in server;			// Устанавливаем порт и адрес.
  server.sin_family = AF_INET;
  server.sin_port = htons(port);

  int ip_addr = argv[1];

  if (inet_pton(AF_INET, ip_addr, &server.sin_addr) <= 0) {	// Получаем адрес сервера.
    perror("Failed to get an addres");
    exit(-1);
  }

  if (connect(client_fd, (SA *)&server, sizeof(server)) < 0) {	// Соединяемся с сервером.
    perror("Failed to connect with server");
    exit(-1);
  }

  printf("Successfully connected to server.\n");

  while (strcmp(buffer, "The End") != 0) {	// Очищаем буфер.
    memset(buffer, 0, sizeof(buffer));
    ssize_t read_bytes = read(client_fd, buffer, BUFFER_SIZE);		// Получаем сообщение от сервера.
    printf("%s\n", buffer);
  }

  printf("Receiver client is off\n");			// Выключаем клиент, освобождаем память.
  close(client_fd);
  return 0;
}
