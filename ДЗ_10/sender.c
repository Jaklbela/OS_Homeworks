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

