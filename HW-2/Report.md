# Жмурина Ксения Игоревна БПИ218,  вариант 34
## Узнав о планах преступников по шифрованию текстов, Шерлок Холмс предложил лондонской полиции специальную машину для дешифровки сообщений злоумышленников.Реализовать приложение, дешифрующее кодированный текст. В качестве ключа используется известная кодовая таблица, устанавливающая однозначное соответствие между каждой буквой и каким-нибудь числом. Процессом узнавания кода в решении задачи пренебречь. Каждый процесс–дешифровщик декодирует свои кусочки текста, многократно получаемые от менеджера. Распределение фрагментов текста между процессами–дешифровщиками осуществляется процессом–менеджером, который передает каждому процессу фрагмент зашифрованного текста, получает от него результат, передает следующий зашифрованный фрагмент. Он же собирает из отдельных фрагментов зашифрованный текст. Количество процессов задается опционально. Каждый процесс может выполнять свою работу за случайное время.
## Программа принимает на вход имена файлов входных и выходных данных а также количество создаваемых процессов. Отдельно есть методы, декодирующие сообщения, считывающие числа с файла закодированного сообщения и обработчик сигналов. Таблица задается заранее уже в программе и не меняется.. Сама программа создает в цикле несколько процессов, которые дешифруют кусочки текста, основываясь на позиции в разделяемой памяти.. Затем программа ждет, пока все процессы отрабатывают и записывает получившийся буфер в файл.
## Оценка 4. Именованные POSIX семафоры.
### Код на си:
```c
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
```
## Оценка 5. Неименованные POSIX-семафоры.
### Код на си
```c
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
```
## Оценка 6. Семафоры System V
### Код на си
```c
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

void parentSignalHandler(int signal) {
    printf("Receive signal");
    for (int i = 0; semid != -1 && i < 2 * pr_num; ++i) {
        semctl(semid, i, IPC_RMID);
    }
    printf("Semaphores deleted\n");
    if (msg_adr != NULL) {
        shmdt(msg_adr);
    }
    if (shmid != -1) {
        shmctl(shmid, IPC_RMID, NULL);
    }
    printf("Shared memory closed\n");
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

    shm_key = ftok("main.c", 0);
    if((shmid = shmget(shm_key, sizeof(message) * pr_num, 0666 | IPC_CREAT | IPC_EXCL)) < 0)  {
        if((shmid = shmget(shm_key, sizeof(message) * pr_num, 0)) < 0) {
            printf("Can\'t connect to shared memory\n");
            exit(-1);
        }
        msg_adr = shmat(shmid, NULL, 0);
        printf("Connected to shared memory\n");
    } else {
        msg_adr = shmat(shmid, NULL, 0);
        printf("New shared memory created\n");
    }

    if ((semid = semget(shm_key, 2 * pr_num, 0666 | IPC_CREAT | IPC_EXCL)) < 0) {
        if ((semid = semget(shm_key, 2 * pr_num, 0)) < 0) {
            printf("Can\'t connect to semaphore\n");
            exit(-1);
        }
        printf("Connected to semaphore successfully\n");
    } else {
        for (int i = 0; i < 2 * pr_num; ++i) {
            semctl(semid, i, SETVAL, 0);
        }
    }

    for (int i = 0; i < pr_num; ++i) {
	pid_t pid = fork();
        if (pid == 0) {			// Код ребенка.
            signal(SIGINT, prev);
            shm_key = ftok("main.c", 0);
    	    if((shmid = shmget(shm_key, sizeof(message) * pr_num, 0666 | IPC_CREAT | IPC_EXCL)) < 0)  {
        	if((shmid = shmget(shm_key, sizeof(message) * pr_num, 0)) < 0) {
                    printf("Can\'t connect to shared memory");
            	    exit(-1);
            	};
                msg_adr = shmat(shmid, NULL, 0);
        	printf("Connect to shared memory\n");
    	    } else {
        	msg_adr = shmat(shmid, NULL, 0);
        	printf("Create new shared memory\n");
    	    }

    	    struct sembuf parent_post = {i + pr_num, 1, 0};
    	    struct sembuf child_wait = {i, -1, 0};
    	    char buffer[4000];
    	    for (;;) {
        	semop(semid, &child_wait, 1);
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
        	semop(semid, &parent_post, 1);
    	    }
    	    semop(semid, &parent_post, 1);
            exit(0);
        }
    }
    
					// Код родителя.
    char decoded[100000];
    int ind = 0;
    int file = open(input, O_RDONLY, S_IRWXU);
    struct sembuf child_post = {0, 1, 0};
    struct sembuf parent_wait = {0, -1, 0};

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
            child_post.sem_num = i;
            semop(semid, &child_post, 1);
	    ++quantity;
        }
        for (int i = 0; i < quantity; ++i) {
            parent_wait.sem_num = i + pr_num;
            semop(semid, &parent_wait, 1);

            for (int j = 0; j < msg_adr[i].size; ++j) {
                decoded[ind] = msg_adr[i].uncoded[j];
		++ind_dec
            }
        }
    }
    for (int i = 0; i < pr_num; ++i) {
        msg_adr[i].type = 3;
        child_post.sem_num = i;
        semop(semid, &child_post, 1);
    }
    for (int i = 0; i < pr_num; ++i) {
        parent_wait.sem_num = i + pr_num;
        semop(semid, &parent_wait, 1);
    }
    decoded[ind] = '\0';
    close(file);

    file = open(output, O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU);
    write(file, decoded, sizeof(char) * ind);
    close(file);

    						// Освобождаем память.
    for (int i = 0; i < 2 * pr_num; ++i) {
        semctl(semid, i, IPC_RMID);
    }
    printf("All semaphores deleted.\n");
    shmdt(msg_adr);
    shmctl(shmid, IPC_RMID, NULL);
    printf("Shared memory deleted\n");
    return 0;
}
```
