#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include "Usuarios.h"
#include "Banco.h"

Config configuracion;

// Estructura de configuración
typedef struct Config {
    int limite_retiro;
    int limite_transferencia;
    int umbral_retiros;
    int umbral_transferencias;
    int num_hilos;
    char archivo_cuentas[50];
    char archivo_log[50];
} Config;

// Leer configuración desde archivo
Config leer_configuracion(const char *ruta) {
    FILE *archivo = fopen(ruta, "r");
    if (archivo == NULL) {
        perror("Error al abrir config.txt");
        exit(1);
    }
    Config config;
    char linea[100];
    while (fgets(linea, sizeof(linea), archivo)) {
        if (linea[0] == '#' || strlen(linea) < 3) continue;
        if (strstr(linea, "LIMITE_RETIRO")) sscanf(linea, "LIMITE_RETIRO=%d", &config.limite_retiro);
        else if (strstr(linea, "LIMITE_TRANSFERENCIA")) sscanf(linea, "LIMITE_TRANSFERENCIA=%d", &config.limite_transferencia);
        else if (strstr(linea, "UMBRAL_RETIROS")) sscanf(linea, "UMBRAL_RETIROS=%d", &config.umbral_retiros);
        else if (strstr(linea, "UMBRAL_TRANSFERENCIAS")) sscanf(linea, "UMBRAL_TRANSFERENCIAS=%d", &config.umbral_transferencias);
        else if (strstr(linea, "NUM_HILOS")) sscanf(linea, "NUM_HILOS=%d", &config.num_hilos);
        else if (strstr(linea, "ARCHIVO_CUENTAS")) sscanf(linea, "ARCHIVO_CUENTAS=%s", config.archivo_cuentas);
        else if (strstr(linea, "ARCHIVO_LOG")) sscanf(linea, "ARCHIVO_LOG=%s", config.archivo_log);
    }
    fclose(archivo);
    return config;
}

int main() {
    Config configuracion = leer_configuracion("config.txt");
    printf("Archivo de cuentas: %s\n", configuracion.archivo_cuentas);
    printf("Número máximo de hilos: %d\n", configuracion.num_hilos);

    // Creación del semáforo
    sem_t *semaforo = sem_open("/cuentas_sem", O_CREAT, 0644, 1);
    if (semaforo == SEM_FAILED) {
        perror("Error al crear el semáforo");
        exit(1);
    }

    // Bucle de espera de conexiones
    while (1) {
        printf("Esperando conexión de usuario...\n");
        sleep(2);
        pid_t pid = fork();
        if (pid == 0) {
            // Código del proceso hijo
            printf("Nuevo usuario conectado. Iniciando sesión...\n");
            ejecutar_menu_usuario();
            exit(0);
        }
    }

    sem_unlink("/cuentas_sem");
    return 0;
}