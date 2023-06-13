# Жмурина Ксения Игоревна БПИ218, ИДЗ-4,  вариант 34, задание на оценку 7
## Узнав о планах преступников по шифрованию текстов, Шерлок Холмс предложил лондонской полиции специальную машину для дешифровки сообщений злоумышленников. Реализовать приложение, дешифрующее кодированный текст.
В качестве ключа используется известная кодовая таблица, устанавливающая однозначное соответствие между каждой буквой и каким-нибудь числом. Процессом узнавания кода в решении задачи пренебречь. Каждый процесс
дешифровщик декодирует свои кусочки текста, многократно получаемые от менеджера. Распределение фрагментов текста между процессами–дешифровщиками осуществляется процессом–менеджером, который передает каждому процессу фрагмент зашифрованного текста, получает от него результат, передает следующий зашифрованный фрагмент. Он же собирает из отдельных фрагментов зашифрованный текст. Количество процессов задается опционально. Каждый процесс может выполнять свою работу за случайное время.Сервер — процесс–менеджер. Клиенты —процессы–дешифровщики.
## Оценка 4-5. Клиенты отображают информацию независимо.
## Клиент принимает на вход IP-адрес и порт сервера, после чего связывается с ним. Получая от сервера кусочки текста, клиент дешифрует их и отправляет обратно. Сервер получает на вход имя файла с зашифрованным текстом, имя файла для вывода, а так же адреса и порт для связи с клиентом. После чего он считывает текст из файла и кусочками его отправляет клиенту. Если после пяти запросов клиент не отвечает, сервер заканчивает работу. Отработав, сервер закрывает файлы, останавливает процессы. Клиент и сервер закрывают сокеты.
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
#include <sys/wait.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

#define MAX_SIZE 4000
#define MAXPENDING 5        // Максимум запросов на соединение.

void (*prev)(int);
char *sh_memo = "shared_memory";
int shmid = -1;
int pr_num;

// Структура сообщения, с которым идет работа.
typedef struct {
    int type;
    int size;
    sem_t child_sem;
    sem_t parent_sem;
    union {
        char uncoded[MAX_SIZE * sizeof(int)];
        int coded[MAX_SIZE];
    };
} message;

message *msg_adr = NULL;      // Адрес сообщения в разделяемой памяти.

int createServerSocket(unsigned short port) {
    int socket_d;
    struct sockaddr_in serv_addr;       // Локальный адрес сокета.

    // Создаем сокет для входящих сообщений.
    if ((socket_d = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
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

    // Помечаем сокет как слушателя входящих сообщений.
    if (listen(socket_d, MAXPENDING) < 0) {
        printf("Listen failed");
        exit(-1);
    }

    return socket_d;
}

void handleClient(int server_socket, int id) {
    int client_socket;
    struct sockaddr_in client_addr;

    unsigned int client_len = sizeof(client_addr);

    // Ожидаем клиент для подключения.
    if ((client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_len)) < 0) {
        printf("Failed to connect with client");
        exit(-1);
    }
    printf("Connected with client successfully\n");

    char buffer[MAX_SIZE];
    sem_post(&msg_adr[id].parent_sem);

    while (1) {
        sem_wait(&msg_adr[id].child_sem);
        if (msg_adr[id].type == 3) {
            break;
        }
        if (send(client_socket, &msg_adr[id].coded, msg_adr[id].size * 4, 0) != msg_adr[id].size * 4) {
            printf("Message send failed");
            exit(-1);
        }
        sleep(2);

        int received_size;
        if ((received_size = recv(client_socket, buffer, MAX_SIZE, 0)) < 0) {
            printf("Receiving failed");
            exit(-1);
        }
        msg_adr[id].size = received_size;
        for (int i = 0; i < msg_adr[id].size; ++i) {
            msg_adr[id].uncoded[i] = buffer[i];
        }
        msg_adr[id].type = 2;
        sem_post(&msg_adr[id].parent_sem);
    }
    close(shmid);
    close(client_socket);
    close(server_socket);
    sem_post(&msg_adr[id].parent_sem);
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



void parentSignalHandler(int signal){
    printf("Receive signal");
    for (int i = 0; msg_adr != NULL && i < pr_num; ++i) {
        sem_destroy(&msg_adr[i].child_sem);
        sem_destroy(&msg_adr[i].parent_sem);
    }
    printf("Children semaphores closed\n");
    printf("Parent semaphore closed.\n");
    if ((shmid = shm_open(sh_memo, O_CREAT | O_RDWR, S_IRWXU)) == -1) {
        if (shm_unlink(sh_memo) == -1) {
            perror("shm_unlink");
            printf("Error getting pointer to shared memory");
            exit(-1);
        }
    }
    printf("Shared memory closed.\n");
    prev(signal);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Wrong arguments");
        exit(-1);
    }


    unsigned short server_port = atoi(argv[4]);         // Порт сервера.
    int serv_sock = createServerSocket(server_port);    // Сокет сервера.
    pr_num = atoi(argv[1]);

    if ((shmid = shm_open(sh_memo, O_CREAT | O_RDWR, S_IRWXU)) == -1) {
        printf("File is already open");
        exit(-1);
    } else {
        printf("File is opened.\n");
    }

    if (ftruncate(shmid, sizeof(message) * pr_num) == -1) {     // Выделяем память.
        printf("Memory sizing error");
        exit(-1);
    }

    msg_adr = mmap(0, sizeof(message) * pr_num + sizeof(observer), PROT_WRITE | PROT_READ, MAP_SHARED, shmid, 0);
    obs_adr = mmap(0, sizeof(message) * pr_num + sizeof(observer), PROT_WRITE | PROT_READ, MAP_SHARED, shmid, 0)
            + sizeof(message) * pr_num;

    if (sem_init(&obs_adr->child_sem, 1, 0) == -1) {
        printf("Failed to create child observer semaphore");
        exit(-1);
    }
    if (sem_init(&obs_adr->parent_sem, 1, 0) == -1) {
        printf("Failed to create parent observer semaphore");
        exit(-1);
    }

    for (int i = 0; i < pr_num; ++i) {
        if (sem_init(&msg_adr[i].child_sem, 1, 0) == -1) {
            printf("Failed to create child client semaphore");
            exit(-1);
        }
        if (sem_init(&msg_adr[i].parent_sem, 1, 0) == -1) {
            printf("Failed to create parent client semaphore");
            exit(-1);
        }
    }

    prev = signal(SIGINT, parentSignalHandler);

    for (int i = 0; i < pr_num; ++i) {
        pid_t process_id;
        if ((process_id = fork()) < 0) {
            printf("Failed to fork a process");
            exit(-1);
        } else if (process_id == 0) {
            signal(SIGINT, prev);
            handleClient(serv_sock, i);
            exit(0);
        }
    }
    for (int i = 0; i < pr_num; ++i) { // Дожидаемся всех клиентов.
        sem_wait(&msg_adr[i].parent_sem);
    }
    
    int file_in = open(argv[2], O_RDONLY, S_IRWXU);
    int file_out = open(argv[3], O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU);
    int end_of_file = 1;
    while (end_of_file == 1) {
        int running = 0;
        for (int i = 0; i < pr_num; ++i, ++running) {
            int size = 0;
            for (; size < MAX_SIZE; ++size) {
                end_of_file = readInt(file_in, &msg_adr[i].coded[size]);
                if (end_of_file == -1) {
                    break;
                }
            }
            if (size == 0) {
                break;
            }
            msg_adr[i].size = size;
            msg_adr[i].type = 1;
            sem_post(&msg_adr[i].child_sem);
        }

        for (int i = 0; i < running; ++i) {
            sem_wait(&msg_adr[i].parent_sem);
            for (int j = 0; j < msg_adr[i].size; ++j) {
                printf("Encoded message: ");
                printf("%c", msg_adr[i].uncoded[j]);
                write(file_out, &msg_adr[i].uncoded[j], 1);
            }
            printf("\n");
        }
    }
    close(file_in);
    close(file_out);
    
    for (int i = 0; i < pr_num; ++i) {
        msg_adr[i].type = 3;
        sem_post(&msg_adr[i].child_sem);
    }
    for (int i = 0; i < pr_num; ++i) {
        sem_wait(&msg_adr[i].parent_sem);
    }
    for (int i = 0; i < pr_num; ++i) {
        sem_destroy(&msg_adr[i].child_sem);
        sem_destroy(&msg_adr[i].parent_sem);
    }
    close(shmid);
    if (shm_unlink(sh_memo) == -1) {
        printf("Shared memory error");
        exit(-1);
    }
    while (pr_num)              // Закрываем все процессы.
    {
        int process_id = waitpid((pid_t) -1, NULL, WNOHANG);
        if (process_id < 0) {
            printf("Waitpid failed");
        } else if (process_id == 0) {
            break;
        } else {
            pr_num--;
        }
    }
    close(serv_sock);
}
```
## Оценка 6-7. Существует мониторящий клиент
## К серверу и клиентам добавляется так же клиент-наблюдатель, который получает от сервера полностью расшифрованный текст и выводит его. Для этого к серверу отдельно добавляется связь с сокетом наблюдателя и ему также направляется сообщение.
## Код клиента не изменилчя.
## Код наблюдателя на си
```c
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
```
## Код сервера изменился незначительно, пусть будет приложен
```c
#include <sys/wait.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

#define MAX_SIZE 4000
#define MAXPENDING 5        // Максимум запросов на соединение.

void (*prev)(int);
char *sh_memo = "shared_memory";
int shmid = -1;
int pr_num;

// Структура сообщения, с которым идет работа.
typedef struct {
    int type;
    int size;
    sem_t child_sem;
    sem_t parent_sem;
    union {
        char uncoded[MAX_SIZE * sizeof(int)];
        int coded[MAX_SIZE];
    };
} message;

message *msg_adr = NULL;      // Адрес сообщения в разделяемой памяти.

typedef struct {
    int type;
    int size;
    sem_t child_sem;
    sem_t parent_sem;
    char buffer[MAX_SIZE];
} observer;

observer *obs_adr = NULL;      // Адрес сообщения для наблюдателя в разделяемой памяти.

int createServerSocket(unsigned short port) {
    int socket_d;
    struct sockaddr_in serv_addr;       // Локальный адрес сокета.

    // Создаем сокет для входящих сообщений.
    if ((socket_d = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
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

    // Помечаем сокет как слушателя входящих сообщений.
    if (listen(socket_d, MAXPENDING) < 0) {
        printf("Listen failed");
        exit(-1);
    }

    return socket_d;
}

void handleClient(int server_socket, int id) {
    int client_socket;
    struct sockaddr_in client_addr;

    unsigned int client_len = sizeof(client_addr);

    // Ожидаем клиент для подключения.
    if ((client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_len)) < 0) {
        printf("Failed to connect with client");
        exit(-1);
    }
    printf("Connected with client successfully\n");

    char buffer[MAX_SIZE];
    sem_post(&msg_adr[id].parent_sem);

    while (1) {
        sem_wait(&msg_adr[id].child_sem);
        if (msg_adr[id].type == 3) {
            break;
        }
        if (send(client_socket, &msg_adr[id].coded, msg_adr[id].size * 4, 0) != msg_adr[id].size * 4) {
            printf("Message send failed");
            exit(-1);
        }
        sleep(2);

        int received_size;
        if ((received_size = recv(client_socket, buffer, MAX_SIZE, 0)) < 0) {
            printf("Receiving failed");
            exit(-1);
        }
        msg_adr[id].size = received_size;
        for (int i = 0; i < msg_adr[id].size; ++i) {
            msg_adr[id].uncoded[i] = buffer[i];
        }
        msg_adr[id].type = 2;
        sem_post(&msg_adr[id].parent_sem);
    }
    close(shmid);
    close(client_socket);
    close(server_socket);
    sem_post(&msg_adr[id].parent_sem);
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



void parentSignalHandler(int signal){
    printf("Receive signal");
    for (int i = 0; msg_adr != NULL && i < pr_num; ++i) {
        sem_destroy(&msg_adr[i].child_sem);
        sem_destroy(&msg_adr[i].parent_sem);
    }
    printf("Children semaphores closed\n");
    printf("Parent semaphore closed.\n");
    if ((shmid = shm_open(sh_memo, O_CREAT | O_RDWR, S_IRWXU)) == -1) {
        if (shm_unlink(sh_memo) == -1) {
            perror("shm_unlink");
            printf("Error getting pointer to shared memory");
            exit(-1);
        }
    }
    printf("Shared memory closed.\n");
    prev(signal);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Wrong arguments");
        exit(-1);
    }


    unsigned short server_port = atoi(argv[4]);         // Порт сервера.
    int serv_sock = createServerSocket(server_port);    // Сокет сервера.
    pr_num = atoi(argv[1]);

    if ((shmid = shm_open(sh_memo, O_CREAT | O_RDWR, S_IRWXU)) == -1) {
        printf("File is already open");
        exit(-1);
    } else {
        printf("File is opened.\n");
    }

    if (ftruncate(shmid, sizeof(message) * pr_num) == -1) {     // Выделяем память.
        printf("Memory sizing error");
        exit(-1);
    }

    msg_adr = mmap(0, sizeof(message) * pr_num + sizeof(observer), PROT_WRITE | PROT_READ, MAP_SHARED, shmid, 0);
    obs_adr = mmap(0, sizeof(message) * pr_num + sizeof(observer), PROT_WRITE | PROT_READ, MAP_SHARED, shmid, 0)
            + sizeof(message) * pr_num;

    if (sem_init(&obs_adr->child_sem, 1, 0) == -1) {
        printf("Failed to create child observer semaphore");
        exit(-1);
    }
    if (sem_init(&obs_adr->parent_sem, 1, 0) == -1) {
        printf("Failed to create parent observer semaphore");
        exit(-1);
    }

    for (int i = 0; i < pr_num; ++i) {
        if (sem_init(&msg_adr[i].child_sem, 1, 0) == -1) {
            printf("Failed to create child client semaphore");
            exit(-1);
        }
        if (sem_init(&msg_adr[i].parent_sem, 1, 0) == -1) {
            printf("Failed to create parent client semaphore");
            exit(-1);
        }
    }

    prev = signal(SIGINT, parentSignalHandler);

    for (int i = 0; i < pr_num; ++i) {
        pid_t process_id;
        if ((process_id = fork()) < 0) {
            printf("Failed to fork a process");
            exit(-1);
        } else if (process_id == 0) {
            signal(SIGINT, prev);
            handleClient(serv_sock, i);
            exit(0);
        }
    }
    for (int i = 0; i < pr_num; ++i) { // Дожидаемся всех клиентов.
        sem_wait(&msg_adr[i].parent_sem);
    }

    pid_t observer_id;
    if ((observer_id = fork()) < 0) {
        printf("Failed to fork an observer process");
        exit(-1);
    } else if (observer_id == 0) {
        signal(SIGINT, prev);
        int observer_socket;
        struct sockaddr_in observer_addr;

        unsigned int observer_len = sizeof(observer_addr);

        // Ожидаем наблюдателя для подключения.
        if ((observer_socket = accept(serv_sock, (struct sockaddr *) &observer_addr, &observer_len)) < 0) {
            printf("Failed to connect with observer");
            exit(-1);
        }
        printf("Connected with observer successfully\n");

        sem_post(&obs_adr->parent_sem);

        while (1) {
            sem_wait(&obs_adr->child_sem);
            if (obs_adr->type == 3) {
                break;
            }
            int size = strlen(obs_adr->buffer);
            if (send(observer_socket, &obs_adr->buffer, size, 0) != size) {
                printf("Failed to send a message to observer");
                exit(-1);
            }
            sem_post(&obs_adr->parent_sem);
        }
        close(shmid);
        close(observer_socket);
        close(serv_sock);
        sem_post(&obs_adr->parent_sem);
        exit(0);
    }
    
    

    int file_in = open(argv[2], O_RDONLY, S_IRWXU);
    int file_out = open(argv[3], O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU);
    int end_of_file = 1;
    while (end_of_file == 1) {
        int running = 0;
        for (int i = 0; i < pr_num; ++i, ++running) {
            int size = 0;
            for (; size < MAX_SIZE; ++size) {
                end_of_file = readInt(file_in, &msg_adr[i].coded[size]);
                if (end_of_file == -1) {
                    break;
                }
            }
            if (size == 0) {
                break;
            }
            msg_adr[i].size = size;
            msg_adr[i].type = 1;
            sem_post(&msg_adr[i].child_sem);
            sem_post(&obs_adr->child_sem);
            sem_wait(&obs_adr->parent_sem);
        }

        for (int i = 0; i < running; ++i) {
            sem_wait(&msg_adr[i].parent_sem);
            for (int j = 0; j < msg_adr[i].size; ++j) {
                printf("Encoded message: ");
                printf("%c", msg_adr[i].uncoded[j]);
                write(file_out, &msg_adr[i].uncoded[j], 1);
                sprintf(obs_adr->buffer, "%s", msg_adr[i].uncoded[j]);
            }
            printf("\n");
            sem_post(&obs_adr->child_sem);
            sem_wait(&obs_adr->parent_sem);
        }
    }
    close(file_in);
    close(file_out);

    obs_adr->type = 3;
    sem_post(&obs_adr->child_sem);
    sem_wait(&obs_adr->parent_sem);
    sem_destroy(&obs_adr->parent_sem);
    sem_destroy(&obs_adr->child_sem);
    for (int i = 0; i < pr_num; ++i) {
        msg_adr[i].type = 3;
        sem_post(&msg_adr[i].child_sem);
    }
    for (int i = 0; i < pr_num; ++i) {
        sem_wait(&msg_adr[i].parent_sem);
    }
    for (int i = 0; i < pr_num; ++i) {
        sem_destroy(&msg_adr[i].child_sem);
        sem_destroy(&msg_adr[i].parent_sem);
    }
    close(shmid);
    if (shm_unlink(sh_memo) == -1) {
        printf("Shared memory error");
        exit(-1);
    }
    while (pr_num)              // Закрываем все процессы.
    {
        int process_id = waitpid((pid_t) -1, NULL, WNOHANG);
        if (process_id < 0) {
            printf("Waitpid failed");
        } else if (process_id == 0) {
            break;
        } else {
            pr_num--;
        }
    }
    close(serv_sock);
}
```
