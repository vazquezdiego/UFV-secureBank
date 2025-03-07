#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include "Usuarios.h"

int main() {
    int pipe_hijo_padre[2];  // Pipe para comunicación hijo → padre
    if (pipe(pipe_hijo_padre) == -1) {
        perror("Error al crear el pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork(); 
    
    if (pid < 0) {
        perror("Error al crear el proceso hijo");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Código del proceso hijo
        close(pipe_hijo_padre[1]);  // Cerrar el extremo de escritura en el hijo
        ejecutar_menu_usuario(pipe_hijo_padre[0]);  // Pasar extremo de lectura
        close(pipe_hijo_padre[0]);
        exit(EXIT_SUCCESS);
    } else {
        // Código del proceso padre 
        close(pipe_hijo_padre[0]);  // Cerrar el extremo de lectura en el padre

        int operacion = 1;  // Ejemplo de operación enviada
        write(pipe_hijo_padre[1], &operacion, sizeof(operacion));

        close(pipe_hijo_padre[1]);  // Cerrar el extremo de escritura
        wait(NULL);  // Esperar a que el hijo termine
    }

    return 0;
}
