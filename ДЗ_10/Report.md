# Жмурина Ксения Игоревна, БПИ218, ДЗ10
## Программа представляет из себя отправителя и получателя, работающих одновременно. Отправитель запрашивает PID получателя, а затем просит пользователя ввести число. Это число отправляется получателю. Программы должна быть запущены одновременно в двух терминалах.
### Код отправителя на си
```c
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

void transmit(int rec_pid, int bit) {
    if (bit == 0) {
        kill(rec_pid, SIGUSR1);
    } else {
        kill(rec_pid, SIGUSR2);
    }
    usleep(1000);
}

int main() {
    int num;
    int rec_pid;
    printf("Transmitter PID is: %d\n", getpid());
    printf("Enter receiver PID: ");
    scanf("%d", &rec_pid);
    printf("Enter a number: ");
    scanf("%d", &num);
    for (int i = 0; i < 30; ++i) {
        transmit(rec_pid, (num >> i) & 1);
    }
    return 0;
}
```
### Код получателя на си
```c
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

volatile int count = 0;
volatile int num = 0;
struct sigaction sigusr1, sigusr2;

void sigusr1Handler(int signal) {       // Обработчики сигналов.
    count++;
}

void sigusr2Handler(int signal) {
    num = num | (1 << count);
    count++;
}

int main() {
    sigusr1.sa_handler = sigusr1Handler;
    sigusr2.sa_handler = sigusr2Handler;
    sigemptyset(&sigusr1.sa_mask);
    sigemptyset(&sigusr2.sa_mask);
    sigusr1.sa_flags = 0;
    sigusr2.sa_flags = 0;

    if (sigaction(SIGUSR1, &sigusr1, NULL) < 0 || sigaction(SIGUSR2, &sigusr2, NULL) < 0) {
        perror("sigaction");
        printf("Signal error");
        exit(-1);
    }
    printf("Receiver PID is: %d\n", getpid());
    printf("Wait...\n");            // Ожидаем отправителя.
    while (count < 30) {
        pause();
    }
    printf("Received number is: %d\n", num);

    return 0;
}
```
## Пример работы. Заметим, что изначально отправленное число 36 превратилось в 72. Это было вызвано ошибкой в обработчике сигнала SIGUSR2 в коде получателя.
![Снимок экрана 2023-04-24 224544](https://user-images.githubusercontent.com/60582691/234101841-c7d346e9-dd91-4e7b-b206-4031b3a0993d.png)
![Снимок экрана 2023-04-24 224558](https://user-images.githubusercontent.com/60582691/234101892-11ac1df4-4748-48f1-a036-de8a66aa3acc.png)
