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

void *EscucharTuberia(void *arg) {
    char mensaje[100];
    int fdBancoMonitor;
    while (1) {
        fdBancoMonitor = open("fifo_BancoMonitor", O_RDONLY);
        if (fdBancoMonitor == -1) {
            perror("Error al abrir la FIFO");
            exit(1);
        }

        ssize_t bytes_leidos = read(fdBancoMonitor, mensaje, sizeof(mensaje));
        if (bytes_leidos > 0) {
            mensaje[bytes_leidos] = '\0'; // Asegurarse de que el mensaje esté terminado en null
            printf("Mensaje recibido: %s\n", mensaje);
            close(fdBancoMonitor);
        } else if (bytes_leidos == 0) {
            close(fdBancoMonitor);
            break; // FIFO cerrada, salir del bucle
        }
    }
}

int main(int argc, char *argv[]) {
    int limeteRetiros = atoi(argv[1]);
    int limeteTransferencias = atoi(argv[2]);

    pthread_t hilo_escuchar;
    pthread_create(&hilo_escuchar, NULL, EscucharTuberia, NULL);
    pthread_join(hilo_escuchar, NULL);
    

    return 0;
}


