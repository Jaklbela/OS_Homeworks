#include <stdio.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <stdlib.h>

const char *child_sem = "/child-semaphore";
const char *parent_sem = "/parent-semaphore";

int codes[27] = {5, 11, 8, 10, 3, 16, 29, 31, 4, 9, 17,		// Система кодов.
                 38, 42, 54, 6, 12, 28, 39, 36, 7, 14,
                 19, 22, 27, 34, 2, 16};

// Переменные.
void (*prev)(int);
char *sh_memo = "shared-memory";
sem_t **parent_sem_pointer = NULL;
sem_t **child_sem_pointer = NULL;
int pr_num;
int shmid = -1;
key_t shm_key;
int semid = -1;
						// Структура сообщения, с которым идет работа.
typedef struct {
    int type;
    int size;
    sem_t child_sem;
    sem_t parent_sem;
    union {
        char uncoded[4000 * sizeof(int)];
        int coded[4000];
    };
} message;

message *msg_adr = NULL;  		// Адрес сообщения в разделяемой памяти

char encode(int code) {			// Декодер.
    for (int i = 0; i < 27; ++i) {
	if (code == 16) {
	    return ' ';
        } else if (codes[i] == code) {
            return 'a' + i;
        }
    }
    return 0;
}

int readInt(int file, int *c) {		// Считыватель интов с файла.
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

char *getChildSemName(int ind) {	// Метод возвращает имя семафора-ребенка.
    char digit = '0' + ind;
    char res[strlen(child_sem) + 2];
    res[0] = '\0';
    strcat(res, child_sem);
    strcat(res, &digit);
    return res;
}

char *getParentSemName(int ind) {	// Метод возвращает имя семафора-родителя.
    char digit = '0' + ind;
    char res[strlen(parent_sem) + 2];
    res[0] = '\0';
    strcat(res, parent_sem);
    strcat(res, &digit);
    return res;
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

int main(int argc, char **argv) {
    prev = signal(SIGINT, parentSignalHandler);
    
    if (argc != 4) {
        printf("Wrong arguments");
        exit(-1);
    }
    
    pr_num = atoi(argv[3]);
    if (pr_num < 1 || pr_num > 10) {
        printf("Wrong number of processes");
        exit(-1);
    }
    char *input = argv[1];
    char *output = argv[2];

    if ((shmid = shm_open(sh_memo, O_CREAT | O_RDWR, S_IRWXU)) == -1) {
        perror("shm_open");
        printf("Object is already open");
    } else {
        printf("Object is open");
    }
    			// Задаем размер памяти.
    if (ftruncate(shmid, sizeof(message) * pr_num) == -1) {
        perror("ftruncate");
        printf("Memory sizing error");
    } else {
        printf("Memory size set");
    }
    			// Получить доступ к памяти.
    msg_adr = mmap(0, sizeof(message) * pr_num, PROT_WRITE | PROT_READ, MAP_SHARED, shmid, 0);

    for (int i = 0; i < pr_num; ++i) {
        if (sem_init(&msg_adr[i].child_sem, 1, 0) == -1) {
            printf("Can't create child semaphore");
	    exit(-1);
        }
        if (sem_init(&msg_adr[i].parent_sem, 1, 0) == -1) {
            printf("Can't create parent semaphore");
        }
    }

    for (int i = 0; i < pr_num; ++i) {
	pid_t pid = fork();
        if (pid == 0) {			// Код ребенка.
            signal(SIGINT, prev);
            if ((shmid = shm_open(mem_name, O_CREAT | O_RDWR, S_IRWXU)) == -1) {
        	perror("shm_open");
        	printf("Object in child is already open");
		exit(-1);
    	    } else {
        	printf("Object in child is open");
    	    }
    			// Получить доступ к памяти.
    	    msg_adr = mmap(0, sizeof(message) * pr_num, PROT_WRITE | PROT_READ, MAP_SHARED, shmid, 0);
    	    char buffer[4000];
    	    for (;;) {
        	sem_wait(&msg_adr[i].child_sem);
        	if (msg_adr[i].type == 3) {
            	   break;
            	}
            	for (int j = 0; j < msg_adr[i].size; ++j) {
            	    buffer[j] = encode(msg_adr[i].coded[j]);
            	}
            	for (int j = 0; j < msg_adr[i].size; ++j) {
            	    msg_adr[i].uncoded[j] = buffer[j];
            	}

            	msg_adr[i].type = 2;
                sem_post(&msg_adr[i].parent_sem);
    	    }
            close(shmid);
            sem_post(&msg_adr[i].parent_sem);
            exit(0);
        }
    }
    
					// Код родителя.
    char decoded[100000];
    int ind = 0;
    int file = open(input, O_RDONLY, S_IRWXU);
    int flag = 1;
    while (flag == 1) {
        int quantity = 0;
        for (int i = 0; i < pr_num; ++i) {
            int size = 0;
            for (; size < 4000; ++size) {
                flag = readInt(file, &msg_adr[i].coded[size]);
                if (flag == -1) {
                    break;
                }
            }
            if (size == 0) {
                break;
            }
            msg_adr[i].size = size;
            msg_adr[i].type = 1;
            sem_post(&msg_adr[i].child_sem);
	    ++quantity
        }

        for (int i = 0; i < quantity; ++i) {
            sem_wait(&msg_adr[i].parent_sem);
            for (int j = 0; j < msg_adr[i].size; ++j) {
                decoded[ind] = msg_adr[i].uncoded[j];
		++ind;
            }
        }
    }

    for (int i = 0; i < pr_num; ++i) {
        msg_adr[i].type = 3;
        sem_post(&msg_adr[i].child_sem);
    }
    for (int i = 0; i < pr_num; ++i) {
        sem_wait(&msg_adr[i].parent_sem);
    }

    decoded[ind] = '\0';
    close(file);
    file = open(output, O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU);
    write(file, decoded, sizeof(char) * ind);
    close(file);

    for (int i = 0; i < pr_num; ++i) {		// Освобождаем память.
        sem_destroy(&msg_adr[i].child_sem);
        sem_destroy(&msg_adr[i].parent_sem);
    }
    close(shmid);
    if (shm_unlink(sh_memo) == -1) {
        perror("shm_unlink");
        printf("Error getting pointer to shared memory");\
        exit(-1);
    }
    return 0;
}
