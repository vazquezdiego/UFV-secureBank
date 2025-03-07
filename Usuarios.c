#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "Usuarios.h"

typedef struct {
    int tipo_operacion;
    // Puedes agregar más campos si es necesario, por ejemplo, cantidad, cuenta destino, etc.
} DatosOperacion;

void ejecutar_menu_usuario(int pipe_hijo_padre) {
    int operacion;
    read(pipe_hijo_padre, &operacion, sizeof(operacion));  // Leer del pipe
    printf("Operación recibida: %d\n", operacion);

    int opcion;
    while (1) {
        printf("1. Deposito\n2. Retiro\n3. Transferencia\n4. Consulta de saldo\n5. Salir\n");
        scanf("%d", &opcion);
        switch (opcion) {
            case 1:
                printf("Realizando depósito...\n");
                {
                    // Crear hilo para realizar depósito
                    DatosOperacion datos_operacion = {1}; // Asumimos que 1 es depósito
                    pthread_t hilo;
                    pthread_create(&hilo, NULL, ejecutar_operacion, (void *)&datos_operacion);
                    pthread_join(hilo, NULL);
                }
                break;
            case 2:
                printf("Realizando retiro...\n");
                {
                    // Crear hilo para realizar retiro
                    DatosOperacion datos_operacion = {2}; // Asumimos que 2 es retiro
                    pthread_t hilo;
                    pthread_create(&hilo, NULL, ejecutar_operacion, (void *)&datos_operacion);
                    pthread_join(hilo, NULL);
                }
                break;
            case 3:
                printf("Realizando transferencia...\n");
                {
                    // Crear hilo para realizar transferencia
                    DatosOperacion datos_operacion = {3}; // Asumimos que 3 es transferencia
                    pthread_t hilo;
                    pthread_create(&hilo, NULL, ejecutar_operacion, (void *)&datos_operacion);
                    pthread_join(hilo, NULL);
                }
                break;
            case 4:
                printf("Consultando saldo...\n");
                {
                    // Crear hilo para consultar saldo
                    DatosOperacion datos_operacion = {4}; // Asumimos que 4 es consulta de saldo
                    pthread_t hilo;
                    pthread_create(&hilo, NULL, ejecutar_operacion, (void *)&datos_operacion);
                    pthread_join(hilo, NULL);
                }
                break;
            case 5:
                exit(0);
            default:
                printf("Opción no válida\n");
        }
    }
}

void *ejecutar_operacion(void *datos) {
    DatosOperacion *operacion = (DatosOperacion *)datos;
    
    // Realiza la operación correspondiente
    switch (operacion->tipo_operacion) {
        case 1:
            printf("Ejecutando depósito...\n");
            break;
        case 2:
            printf("Ejecutando retiro...\n");
            break;
        case 3:
            printf("Ejecutando transferencia...\n");
            break;
        case 4:
            printf("Consultando saldo...\n");
            break;
        default:
            printf("Operación desconocida...\n");
            break;
    }

    return NULL;
}
