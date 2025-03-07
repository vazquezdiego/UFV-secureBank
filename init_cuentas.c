#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

struct Cuenta {
    int numero_cuenta;
    char titular[50];
    float saldo;
    int num_transacciones;
};

int main() {
    // Crear o abrir el semáforo
    sem_t *semaforo = sem_open("/cuentas_sem", O_CREAT, 0644, 1);
    if (semaforo == SEM_FAILED) {
        perror("Error al crear el semáforo");
        exit(EXIT_FAILURE);
    }

    printf("Semáforo creado correctamente.\n");

    // Aquí iría el código para manejar la estructura Cuenta y el semáforo

    // Cerrar el semáforo al final
    sem_close(semaforo);
    sem_unlink("/cuentas_sem");  // Opcional: eliminar el semáforo si no se necesita más

    return 0;
}
