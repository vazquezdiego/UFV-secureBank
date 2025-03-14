#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "Usuarios.h"
#include "Config.h"

extern sem_t *semaforo;

void realizar_deposito(int cuenta, float monto) {
    if (monto <= 0) {
        printf("Error: Monto de depósito inválido.\n");
        return;
    }
    sem_wait(semaforo);
    printf("Depósito realizado en cuenta %d por %.2f\n", cuenta, monto);
    sem_post(semaforo);
}

void realizar_retiro(int cuenta, float monto) {
    if (monto > configuracion.limite_retiro) {
        printf("Error: Retiro excede el límite permitido (%d)\n", configuracion.limite_retiro);
        return;
    }
    sem_wait(semaforo);
    printf("Retiro realizado en cuenta %d por %.2f\n", cuenta, monto);
    sem_post(semaforo);
}

void realizar_transferencia(int origen, int destino, float monto) {
    if (monto > configuracion.limite_transferencia) {
        printf("Error: Transferencia excede el límite permitido (%d)\n", configuracion.limite_transferencia);
        return;
    }
    sem_wait(semaforo);
    printf("Transferencia de cuenta %d a cuenta %d por %.2f\n", origen, destino, monto);
    sem_post(semaforo);
}

void consultar_saldo(int cuenta) {
    sem_wait(semaforo);
    printf("Saldo de cuenta %d: $%.2f\n", cuenta, 1000.00);
    sem_post(semaforo);
}

void ejecutar_menu_usuario() {
    int opcion, cuenta, destino;
    float monto;
    while (1) {
        printf("1. Depósito\n2. Retiro\n3. Transferencia\n4. Consultar saldo\n5. Salir\n");
        scanf("%d", &opcion);
        switch (opcion) {
            case 1: printf("Ingrese cuenta y monto: "); scanf("%d %f", &cuenta, &monto); realizar_deposito(cuenta, monto); break;
            case 2: printf("Ingrese cuenta y monto: "); scanf("%d %f", &cuenta, &monto); realizar_retiro(cuenta, monto); break;
            case 3: printf("Ingrese cuenta de origen, destino y monto: "); scanf("%d %d %f", &cuenta, &destino, &monto); realizar_transferencia(cuenta, destino, monto); break;
            case 4: printf("Ingrese cuenta: "); scanf("%d", &cuenta); consultar_saldo(cuenta); break;
            case 5: exit(0);
        }
    }
}

