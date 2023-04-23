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

void parentSignalHandler(int signal) {			// Метод, обрабатывающий сигналы для родителя.
    printf("Receive signal");
    for (int i = 0; child_sem_pointer != NULL && i < pr_num; ++i) {
        sem_destroy(child_sem_pointer[i]);
        if (sem_unlink(getChildSemName(i)) == -1) {
            perror("sem_unlink");
            printf("Error getting pointer to semaphore - children");
	    exit(-1);
        }
    }
    printf("Children saemaphores closed\n");
    for (int i = 0; parent_sem_pointer != NULL && i < pr_num; ++i) {
        sem_destroy(parent_sem_pointer[i]);
        if (sem_unlink(getParentSemName(i)) == -1) {
            perror("sem_unlink");
            printf("Error getting pointer to semaphore - parent");
	    exit(-1);
        }
    }
    printf("Parent semaphore closed\n");
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

    sem_t *parent_sems[pr_num];
    sem_t *child_sems[pr_num];

    for (int i = 0; i < pr_num; ++i) {
        parent_sems[i] = sem_open(getParentSemName(i), O_CREAT, 0666, 0);
    }
    for (int i = 0; i < pr_num; ++i) {
        child_sems[i] = sem_open(getChildSemName(i), O_CREAT, 0666, 0);
    }

    parent_sem_pointer = parent_sems;
    child_sem_pointer = child_sems;

    for (int i = 0; i < pr_num; ++i) {
        pid_t pid = fork();       
        if (pid == 0) {
            signal(SIGINT, prev);  // Код ребенка.
            if ((shmid = shm_open(sh_memo, O_CREAT | O_RDWR, S_IRWXU)) == -1) {
        	perror("shm_open");
        	printf("Object in child is already open");
        	exit(-1);
    	    } else {
        	printf("Object in child is open);
    	    }
    		// Получить доступ к памяти.
    	    msg_adr = mmap(0, sizeof(message) * pr_num, PROT_WRITE | PROT_READ, MAP_SHARED, shmid, 0);
    	    char buffer[4000];
    	    for (;;) {
        	sem_wait(child_sems[i]);
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
        	sem_post(pr_sem);
    	    }
    	    close(shmid);
    	    sem_post(parent_sems[i]);
            exit(0);
        }
    }


    char decoded[100000];			// Код родителя.
    int ind = 0;
    if ((shmid = shm_open(sh_memo, O_CREAT | O_RDWR, S_IRWXU)) == -1) {
        perror("shm_open");
        printf("Object in parent is already open");
    } else {
        printf("Object in parent is open");
    }
    			// Задаем размер памяти
    if (ftruncate(shmid, sizeof(message) * pr_num) == -1) {
        perror("ftruncate");
        printf("Memory sizing error - parent");
        exit(-1);
    } else {
        printf("Memory size set");
    }
    				// Получить доступ к памяти.
    msg_adr = mmap(0, sizeof(message) * pr_num, PROT_WRITE | PROT_READ, MAP_SHARED, shmid, 0);

    int file = open(input, O_RDONLY, S_IRWXU);
    int flag = 1;
    while (flag == 1) {
        int quantity = 0;
        for (int i = 0; i < pr_num; ++i, ++quantity) {
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
            sem_post(child_sems[i]);
        }
        for (int i = 0; i < quantity; ++i) {
            sem_wait(parent_sems[i]);
            for (int j = 0; j < msg_adr[i].size; ++j, ++ind) {
                decoded[ind] = msg_adr[i].uncoded[j];
                printf("%c", decoded[ind]);
            }
            printf("\n");
        }
    }
    close(file);
    for (int i = 0; i < pr_num; ++i) {
        msg_adr[i].type = 3;
        sem_post(child_sems[i]);
    }
    for (int i = 0; i < pr_num; ++i) {
        sem_wait(parent_sems[i]);
        sem_close(parent_sems[i]);
        sem_close(child_sems[i]);
    }
    decoded[ind] = '\0';
    printf("%s\n", decoded);
    file = open(output, O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU);
    write(file, decoded, sizeof(char) * ind);
    close(file);
    close(shmid);
    if (shm_unlink(sh_memo) == -1) {
        perror("shm_unlink");
        printf("Error getting pointer to shared memory - parent");
        exit(-1);
    }


    for (int i = 0; i < pr_num; ++i) {				// Освобождение памяти.
        if (sem_unlink(getChildSemName(i)) == -1) {
            perror("sem_unlink");
            printf("Error getting pointer to semaphore - child");
	    exit(-1);
        }
        if (sem_unlink(getParentSemName(i)) == -1) {
            perror("sem_unlink");
            printf("Error getting pointer to semaphore - parent");
	    exit(-1);
        }
    }
    return 0;
}
