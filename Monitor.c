#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>

#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_TEXT 100

struct msgbuf {
    long tipo;
    char texto[MAX_TEXT];
};

void enviar_alerta(const char *mensaje) {
    printf("ALERTA: %s\n", mensaje);
}



int main(int argc, char *argv[]) {
    int limeteRetiros = atoi(argv[1]);
    int limeteTransferencias = atoi(argv[2]);

    return 0;
}


