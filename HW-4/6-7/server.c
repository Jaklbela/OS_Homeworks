#include <unistd.h>
#include <fcntl.h>
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
#include <sys/mman.h>

#define MAX_SIZE 4000

int pr_num;
int socket_d;
struct sockaddr_in serv_addr;
struct sockaddr_in obs_addr;
unsigned int client_len = sizeof(struct sockaddr_in);
unsigned int obs_len = sizeof(struct sockaddr_in);
struct sockaddr_in client_adds[25];

void createServerSocket(unsigned short port) {

    // Создаем сокет для входящих сообщений.
    if ((socket_d = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        printf("Socket error");
        exit(-1);
    }

    // Настраиваем сервер.
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    // Связываемся с адресом.
    if (bind(socket_d, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("Bind failed");
        exit(-1);
    }
}

void getClients() {
    for (int i = 0; i < pr_num; ++i) {
        int msg_size;
        char buffer[4000];
        if ((msg_size = recvfrom(socket_d, buffer, 3999, 0,
                                 (struct sockaddr *) &client_adds[i], &client_len)) < 0) {
            printf("Recieve failed");
            exit(-1);
        }
        buffer[msg_size] = '\0';
        printf("Handling decoder client");
    }
}

void getObserver() {
    int msg_size;
    char buffer[4000];
    if ((msg_size = recvfrom(socket_d, buffer, 3999, 0,
                             (struct sockaddr *) &obs_addr, &obs_len)) < 0) {
        printf("Receiving failed");
        exit(-1);
    }
    buffer[msg_size] = '\0';
    printf("Handling observer client.\n");
}

int readInt(int file, int *c) {    // Считыватель интов с файла.
    char line[10];
    for (int i = 0; i < 10; ++i) {
        int num = read(file, &line[i], sizeof(char));
        if (num == 0 || line[i] == '\n' || line[i] == ' ') {
            line[i] = '\0';
            break;
        }
    }
    if (strlen(line) == 0) {
        return -1;
    }
    sscanf(line, "%d", c);
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Wrong arguments");
        exit(-1);
    }

    int buffer[MAX_SIZE];
    unsigned short serv_port = atoi(argv[4]);         // Порт сервера.
    struct sockaddr_in inp_addr;
    unsigned int inp_size = sizeof(inp_addr);
    int received_bytes;
    int input_file = open(argv[2], O_RDONLY, S_IRWXU);
    int output_file = open(argv[3], O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU);
    pr_num = atoi(argv[1]);

    createServerSocket(serv_port);
    getClients();
    getObserver();

    char message[10000];
    char decoded_part[MAX_SIZE + 1];
    char decoded[(MAX_SIZE + 1) * pr_num];

    int flag = 1;
    while (flag == 1) {
        int running = 0;
        for (int i = 0; i < pr_num; ++i, ++running) {
            int size = 0;

            for (; size < MAX_SIZE; ++size) {
                flag = readInt(input_file, &buffer[size]);
                if (flag == -1) {
                    break;
                }
            }

            if (size == 0) {
                break;
            }

            sprintf(message, "Send message to decoder client: ");
            for (int j = 0; j < size; ++j) {
                sprintf(message + strlen(message), "%d ", buffer[j]);
            }

            sprintf(message + strlen(message), "\n");
            printf("%s", message);

            if (sendto(socket_d, buffer, sizeof(int) * size, 0,
                       (struct sockaddr *) &client_adds[i], client_len) != sizeof(int) * size) {
                printf("Sent a different number of bytes than expected.");
                exit(-1);
            }

            if (sendto(socket_d, message, strlen(message), 0,
                       (struct sockaddr *) &obs_addr, obs_len) != strlen(message)) {
                printf("Sent a different number of bytes than expected.");
                exit(-1);
            }
        }
        sleep(2);
        for (int i = 0; i < running; ++i) {
            if ((received_bytes = recvfrom(socket_d, decoded_part, MAX_SIZE, 0,
                                           (struct sockaddr *) &inp_addr, &inp_size)) < 0) {
                printf("Receiving failed");
                exit(-1);
            }

            decoded_part[received_bytes] = '\0';

            for (int index = 0; index < pr_num; ++index) {
                if (client_adds[index].sin_addr.s_addr == inp_addr.sin_addr.s_addr &&
                    client_adds[index].sin_port == inp_addr.sin_port) {
                    sprintf(message, "Received message from client: ");
                    printf("%s", message);
                    strcpy(&decoded[(MAX_SIZE + 1) * index], decoded_part);
                    if (sendto(socket_d, message, strlen(message), 0,
                               (struct sockaddr *) &obs_addr, obs_len) != strlen(message)) {
                        printf("Sent a different number of bytes than expected");
                        exit(-1);
                    }
                }
            }
        }
        
        for (int i = 0; i < running; ++i) {
            int size = strlen(&decoded[(MAX_SIZE + 1) * i]);
            if (size == 0) {
                break;
            }
            write(output_file, &decoded[(MAX_SIZE + 1) * i], size);
        }
    }
    
    printf("%s", message);
    if (sendto(socket_d, message, strlen(message), 0,
               (struct sockaddr *) &obs_addr, obs_len) != strlen(message)) {
        printf("Sent a different number of bytes than expected");
        exit(-1);
    }
    
    buffer[0] = -1;
    for (int index = 0; index < pr_num; ++index) {
        if (sendto(socket_d, buffer, 5, 0, (struct sockaddr *) &client_adds[index], client_len) != 5) {
            printf("Sent a different number of bytes than expected");
            exit(-1);
        }
    }
    
    message[0] = '\0';
    
    if (sendto(socket_d, message, 1, 0, (struct sockaddr *) &obs_addr, obs_len) != 1) {
        printf("Sent a different number of bytes than expected");
        exit(-1);
    }
    close(input_file);
    close(output_file);
    close(socket_d);
}
