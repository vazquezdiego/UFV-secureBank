#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <unistd.h>

#define MAX_TEXT 100

struct msgbuf {
    long tipo;
    char texto[MAX_TEXT];
};

void enviar_alerta(const char *mensaje) {
    printf("ALERTA: %s\n", mensaje);
}

int main() {
    int cola_mensajes = msgget(1234, 0666 | IPC_CREAT);
    if (cola_mensajes == -1) {
        perror("Error al acceder a la cola de mensajes");
        exit(1);
    }

    struct msgbuf mensaje;
    while (1) {
        if (msgrcv(cola_mensajes, &mensaje, sizeof(mensaje.texto), 0, 0) > 0) {
            printf("Monitoreo: %s\n", mensaje.texto);
            if (strstr(mensaje.texto, "retiros consecutivos") || strstr(mensaje.texto, "transferencias repetitivas")) {
                enviar_alerta(mensaje.texto);
            }
        }
    }

    return 0;
}


