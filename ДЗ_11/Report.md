# Жмурина Ксения Игоревна, БПИ218, ДЗ11
## Программа представляет из себя коды сервера, клиента-отправителя и клиента-получателя, которые запускаются отдельно на разных терминалах или компьютерах, соединяемые по протоколу TCP. Клиент-отправитель получает сообщения, передает их серверу, который, в свобю очередь, передает их клиенту-получателю.
### Код сервера на си
```c
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
```
### Код клиента-отправителя на си
```c
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

  do {					// Читаем сообщение.
    fgets(buffer, BUFFER_SIZE, stdin);
    buffer[strcspn(buffer, "\n")] = 0;
    if (send(client_fd, buffer, strlen(buffer), 0) < 0) {	// Отправляем на сервер.
      perror("Failed to send a message");
      exit(-1);
    }
  } while (strcmp(buffer, "The End") != 0);

  printf("Sender client is off\n");			// Закрываем клиент, освобождаем память.
  close(client_fd);
  return 0;
}
```
### Код клиента-получателя на си
```c
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
```
