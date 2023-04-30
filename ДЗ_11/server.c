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
  if (argc != 2) {
    printf("Wrong arguments");
    exit(-1);
  }

  char buffer[BUFFER_SIZE];

  int server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);	// Создаем TCP сокет.
  if (server_fd == 0) {
    perror("Failed to create a socket");
    exit(-1);
  }

  int port = atoi(argv[1]);

  struct sockaddr_in server;				// Определяем адрес и порт сервера.
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(port);

  if (bind(server_fd, (SOCKET *)&server, sizeof(server)) < 0) {	// Связываем сокет и порт.
    perror("Failed to bind socket and port");
    exit(-1);
  }

  if (listen(server_fd, 2) < 0) {		// Слушаем связь.
    perror("Failed to listen for connections");
    exit(-1);
  }

  int client_fst_fd;				// Связываемся с первым клиентом.
  struct sockaddr_in client_fst_addr;
  int client_fst_size = sizeof(client_fst_addr);
  if ((client_fst_fd = accept(server_fd, (SOCKET *)&client_fst_addr,
                           (socklen_t *)&client_fst_size)) < 0) {
    perror("Failed to connect with first client");
    exit(-1);
  }

  int client_snd_fd;				// Связываемся со вторым клиентом.
  struct sockaddr_in client_snd_addr;
  int client_snd_size = sizeof(client_snd_addr);
  if ((client_snd_fd = accept(server_fd, (SOCKET *)&client_snd_addr,
                           (socklen_t *)&client_snd_size)) < 0) {
    perror("Failed to connect with second client");
    exit(-1);
  }

  printf("Succesfully connected between first and second client.\n");

  ssize_t read_bytes;				// Читаем и перенаправляем сообщения.
  while ((read_bytes = read(client_fst_fd, buffer, BUFFER_SIZE)) > 0) {
    printf("Message received");
    if (send(client_snd_fd, buffer, read_bytes, 0) < 0) {	// Отправляем сообщение.
      perror("Failed to send message to second client");
      exit(-1);
    }
    printf("Message has been sent.\n");
    if (strcmp(buffer, "The End") == 0) {		// Заканчиваем получение при терминальном сообщении.
      break;
    }
    memset(buffer, 0, BUFFER_SIZE);
  }

  printf("Server is off\n");			// Выключаем сервер, освобождаем данные.
  close(client_fst_fd);
  close(client_snd_fd);
  close(server_fd);
  return 0;
}
