# Жмурина Ксения Игоревна БПИ218,  вариант 34
## Узнав о планах преступников по шифрованию текстов, Шерлок Холмс предложил лондонской полиции специальную машину для дешифровки сообщений злоумышленников.Реализовать приложение, дешифрующее кодированный текст. В качестве ключа используется известная кодовая таблица, устанавливающая однозначное соответствие между каждой буквой и каким-нибудь числом. Процессом узнавания кода в решении задачи пренебречь. Каждый процесс–дешифровщик декодирует свои кусочки текста, многократно получаемые от менеджера. Распределение фрагментов текста между процессами–дешифровщиками осуществляется процессом–менеджером, который передает каждому процессу фрагмент зашифрованного текста, получает от него результат, передает следующий зашифрованный фрагмент. Он же собирает из отдельных фрагментов зашифрованный текст. Количество процессов задается опционально. Каждый процесс может выполнять свою работу за случайное время.
## Программа принимает на вход имена файлов входных и выходных данных а также количество создаваемых процессов. Отдельно есть методы, регулирующие работу семафор (используются семафоры System V), а также декодеры. Сама программа создает в цикле несколько процессов, которые дешифруют кусочки текста, основываясь на номере процесса и размеру обрабатываемого отрывка. Затем программа ждет, пока все процессы отрабатывают и записывает получившийся буфер в файл.
## Оценка 4. Семафоры System V.
###Код на си:
```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>

#define BUFFER_SIZE 1024
#define SEM_KEY 12345

int codes[27] = {5, 11, 8, 10, 3, 16, 29, 31, 4, 9, 17,		// Система кодов.
                 38, 42, 54, 6, 12, 28, 39, 36, 7, 14,
                 19, 22, 27, 34, 2, 16};

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

struct sembuf grab_sem = {0, -1, SEM_UNDO}; 			// Ожидание.
struct sembuf release_sem = {0, 1, SEM_UNDO}; 			// Сигнал.

int sem_id;
unsigned char *input_buffer, *output_buffer;
int input_size, output_size;
int chunk_size;

void grab_semaphore(int sem_num) {
    grab_sem.sem_num = sem_num;
    semop(sem_id, &grab_sem, 1);
}

void release_semaphore(int sem_num) {
    release_sem.sem_num = sem_num;
    semop(sem_id, &release_sem, 1);
}

void decode_chunk(unsigned char *input_buf, unsigned char *output_buf, int start_pos) {		// Декодер.
    for (int i = 0; i < chunk_size; i++) {
        int input_pos = start_pos + i;
        int output_pos = start_pos + i;
        for (int j = 0; j < 27; ++j) {
            if (input_buf[input_pos] == codes[j]) {
                if (j == 26) {
                    output_buf[output_pos] = ' ';
                } else {
                    output_buf[output_pos] = (i + 97) + '0';
                }
            }
        }
    }
}

void decode_intensive(int process_id, int quantity) {			// Декодирование с учетом семафоров.
    int start_pos = process_id * chunk_size;
    int end_pos = (process_id + 1) * chunk_size;
    if (end_pos > input_size) end_pos = input_size;
    while (start_pos < end_pos) {
        grab_semaphore(process_id + 1); 				// Ждем семафору.
        decode_chunk(input_buffer, output_buffer, start_pos);
        release_semaphore(0); 						// Сигнал для главного процесса.
        start_pos += chunk_size * (quantity + 1);
    }
    exit(0);
}

int main(int argc, char **argv) {
    if (argc != 4) {
        printf("Wrong arguments");
        exit(-1);
    }
    
    int qunatity = atoi(argv[3]);
    if (qunatity < 1 || qunatity > 10) {
        printf("Wrong number of processes");
        exit(-1);
    }
    char *input_file = argv[1];
    char *output_file = argv[2];

    									// Считываем файл.
    FILE *fp = fopen(input_file, "rb");
    fseek(fp, 0L, SEEK_END);
    input_size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    input_buffer = (unsigned char *) malloc(input_size * sizeof(unsigned char));
    fread(input_buffer, input_size, 1, fp);
    fclose(fp);

    									// Инициализируем выходной буфер.
    output_size = input_size; 						// Размер берем с учетом худшего сценария.
    output_buffer = (unsigned char *) malloc(output_size * sizeof(unsigned char));
    
    									// ИНициализация семафор.
    sem_id = semget(SEM_KEY, qunatity + 1, IPC_CREAT | 0666);
    if (sem_id < 0) {
        printf("Could not create semaphore.\n");
        exit(-1);
    }
    union semun sem_arg;
    sem_arg.val = 0; 							// Инициализируем декодеры как 0, главный процесс как 1.
    if (semctl(sem_id, 0, SETVAL, sem_arg) < 0) {
        printf("Could not initialize the first semaphore.\n");
        exit(-1);
    }
    sem_arg.val = 1; 							// Все декодеры ждут главный процесс.
    for (int i = 1; i < qunatity + 1; i++) {
        if (semctl(sem_id, i, SETVAL, sem_arg) < 0) {
            printf("Could not initialize a worker semaphore.\n");
            exit(-1);
        }
    }

    									// Инициализируем размер отрывков.
    chunk_size = input_size / qunatity;
    for (int i = 1; i < qunatity + 1; i++) {
        pid_t pid = fork();
        if (pid >= 0) { 					// Ребенок.
            decode_intensive(i, qunatity);
            sleep(1);
        } else if (pid < 0) { 					// Случай ошибки.
            printf("Could not create a child process.\n");
            exit(-1);
        }
    }

    								// Главный процесс.
    int start_pos = 0;
    while (start_pos < input_size) {
        for (int i = 0; i < qunatity; i++) {
            grab_semaphore(0); 					// Ожидаем семафору.
            release_semaphore(i + 1); 				// Выпускаем декодер.
        }
        grab_semaphore(0); 					// Убеждаемся, что выпустили все декодеры.
        for (int i = 0; i < qunatity; i++) {
            release_semaphore(i + 1); 				// Сбрасываем декодеры.
        }
        start_pos += chunk_size;
    }

    								// Ждем, пока декодеры закончат.
    for (int i = 0; i < qunatity; i++) {
        wait(NULL);
    }

    								// Сохраняем файл.
    fp = fopen(output_file, "wb");
    fwrite(output_buffer, sizeof(unsigned char), input_size, fp);
    fclose(fp);

    								// Освобождаем память.
    semctl(sem_id, 0, IPC_RMID);
    free(input_buffer);
    free(output_buffer);

    return 0;
}
```
