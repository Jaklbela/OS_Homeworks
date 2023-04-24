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
