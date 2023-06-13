# Жмурина Ксения Игоревна БПИ218, ИДЗ-4,  вариант 34, задание на оценку 7
## Узнав о планах преступников по шифрованию текстов, Шерлок Холмс предложил лондонской полиции специальную машину для дешифровки сообщений злоумышленников. Реализовать приложение, дешифрующее кодированный текст.
В качестве ключа используется известная кодовая таблица, устанавливающая однозначное соответствие между каждой буквой и каким-нибудь числом. Процессом узнавания кода в решении задачи пренебречь. Каждый процесс
дешифровщик декодирует свои кусочки текста, многократно получаемые от менеджера. Распределение фрагментов текста между процессами–дешифровщиками осуществляется процессом–менеджером, который передает каждому процессу фрагмент зашифрованного текста, получает от него результат, передает следующий зашифрованный фрагмент. Он же собирает из отдельных фрагментов зашифрованный текст. Количество процессов задается опционально. Каждый процесс может выполнять свою работу за случайное время.Сервер — процесс–менеджер. Клиенты —процессы–дешифровщики.
## Оценка 4-5. Клиенты отображают информацию независимо.
## Клиент принимает на вход IP-адрес и порт сервера, после чего связывается с ним. Получая от сервера кусочки текста, клиент дешифрует их и отправляет обратно. Сервер получает на вход имя файла с зашифрованным текстом, имя файла для вывода, а так же адреса и порт для связи с клиентом. После чего он считывает текст из файла и кусочками его отправляет клиенту.  Отработав, сервер закрывает файлы, останавливает процессы. Клиент и сервер закрывают сокеты.
### Код клиента на си:
```c
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
```

### Код сервера на си
```c
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
unsigned int client_len = sizeof(struct sockaddr_in);
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
    unsigned short serv_port = atoi(argv[4]);;
    struct sockaddr_in inp_addr;
    unsigned int inp_size = sizeof(inp_addr);
    int received_bytes;
    int input_file = open(argv[2], O_RDONLY, S_IRWXU);
    int output_file = open(argv[3], O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU);
    pr_num = atoi(argv[1]);

    createServerSocket(serv_port);
    getClients();

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

            for (int j = 0; j < size; ++j) {
                printf("Send message to client: ");
                printf("%d ", buffer[j]);
            }

            printf("\n");
            if (sendto(socket_d, buffer, sizeof(int) * size, 0,
                       (struct sockaddr *) &client_adds[i], client_len) != sizeof(int) * size) {
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
            
            for (int ind = 0; ind < pr_num; ++ind) {
                if (client_adds[ind].sin_addr.s_addr == inp_addr.sin_addr.s_addr &&
                    client_adds[ind].sin_port == inp_addr.sin_port) {
                    printf("Received message from client: ");
                    strcpy(&decoded[(MAX_SIZE + 1) * ind], decoded_part);
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
    
    buffer[0] = -1;
    for (int index = 0; index < pr_num; ++index) {
        if (sendto(socket_d, buffer, 4, 0, (struct sockaddr *) &client_adds[index], client_len) != 4) {
            printf("Sent a different number of bytes than expected");
            exit(-1);
        }
    }

    close(input_file);
    close(output_file);
    close(socket_d);
}
```
## Оценка 6-7. Существует мониторящий клиент
## К серверу и клиентам добавляется так же клиент-наблюдатель, который получает от сервера полностью расшифрованный текст и выводит его. Для этого к серверу отдельно добавляется связь с сокетом наблюдателя и ему также направляется сообщение.
## Код клиента не изменилcя.
## Код наблюдателя на си
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
```
## Код сервера на си
```c
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
```
