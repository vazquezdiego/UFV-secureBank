// +------------------------------------------------------------------------------------------------------------------------------------------------+/
//  Archivo: Banco.c
//  Descripción: El programa principal, Banco.c, es el encargado de gestionar la creación de usuarios y la interacción con el monitor.
//
//
// Funciones:
// - EscribirEnLog(const char *mensaje): Abre un archivo de log y escribe el mensaje proporcionado al final de este archivo
// - ObtenerFechaHora(char *buffer, size_t bufferSize): Obtiene la fecha y hora actuales
// - leer_configuracion(const char *ruta): Lee la configuración desde un archivo de texto y devuelve una estructura Config
// - verificarUsuario(const char *archivoLectura, int IdCuenta): Verifica si un usuario existe en el archivo de cuentas
// - MostrarMonitor(void *arg): Crea un proceso hijo que ejecuta el monitor en una nueva terminal
// - MostrarMenu(void *arg): Muestra el menú principal del banco y gestiona la creación de usuarios y la conexión de usuarios existentes
// - EscucharTuberiaMonitor(void *arg): Escucha mensajes de una tubería FIFO y los muestra en la consola
// - main(): Función principal que inicializa la configuración, crea las tuberías y los hilos, y gestiona el flujo del programa
//
// +------------------------------------------------------------------------------------------------------------------------------------------------+/

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
#include <signal.h>
#include <sys/types.h>

// Archivos
#include "Usuarios.h"
#include "Banco.h"
#include "init_cuentas.c"
#include "crearUsuario.h"
#include "Config.h"
#include "Monitor.h"

Config configuracion;

void EscribirEnLog(const char *mensaje)
{
    FILE *archivoLog = fopen(configuracion.archivo_log, "a"); // "a" → Añadir al final
    if (!archivoLog)
    {
        perror("Error al abrir el archivo de log");
        return;
    }

    fprintf(archivoLog, "%s", mensaje);

    fclose(archivoLog);
}

// Como llamamos a la funcion de obtener la fecha y la hora
// ObtenerFechaHora(FechaHora, sizeof(FechaHora));
void ObtenerFechaHora(char *buffer, size_t bufferSize)
{
    time_t t;
    struct tm *tm_info;

    time(&t);
    tm_info = localtime(&t);

    strftime(buffer, bufferSize, "%Y-%m-%d %H:%M:%S", tm_info);
}

// Leer configuración desde archivo
Config leer_configuracion(const char *ruta)
{
    FILE *archivo = fopen(ruta, "r");
    if (archivo == NULL)
    {
        perror("Error al abrir config.txt");
        exit(1);
    }

    Config config;
    char linea[100];

    while (fgets(linea, sizeof(linea), archivo))
    {
        if (linea[0] == '#' || strlen(linea) < 3)
            continue;

        if (strstr(linea, "LIMITE_RETIRO"))
            sscanf(linea, "LIMITE_RETIRO=%d", &config.limite_retiro);
        else if (strstr(linea, "LIMITE_TRANSFERENCIA"))
            sscanf(linea, "LIMITE_TRANSFERENCIA=%d", &config.limite_transferencia);
        else if (strstr(linea, "UMBRAL_RETIROS"))
            sscanf(linea, "UMBRAL_RETIROS=%d", &config.umbral_retiros);
        else if (strstr(linea, "UMBRAL_TRANSFERENCIAS"))
            sscanf(linea, "UMBRAL_TRANSFERENCIAS=%d", &config.umbral_transferencias);
        else if (strstr(linea, "NUM_HILOS"))
            sscanf(linea, "NUM_HILOS=%d", &config.num_hilos);
        else if (strstr(linea, "ARCHIVO_CUENTAS"))
            sscanf(linea, "ARCHIVO_CUENTAS=%s", config.archivo_cuentas);
        else if (strstr(linea, "ARCHIVO_LOG"))
            sscanf(linea, "ARCHIVO_LOG=%s", config.archivo_log);
        else if (strstr(linea, "ARCHIVO_TRANSACCIONES"))
            sscanf(linea, "ARCHIVO_TRANSACCIONES=%s", config.archivo_transacciones);
        else if (strstr(linea, "RUTA_USUARIO"))
            sscanf(linea, "RUTA_USUARIO=%s", config.ruta_usuario);
        else if (strstr(linea, "RUTA_CREARUSUARIO"))
            sscanf(linea, "RUTA_CREARUSUARIO=%s", config.ruta_crearusuario);
        else if (strstr(linea, "RUTA_MONITOR"))
            sscanf(linea, "RUTA_MONITOR=%s", config.ruta_monitor);
        else if (strstr(linea, "MAX_USUARIOS"))
            sscanf(linea, "MAX_USUARIOS=%d", &config.max_usuarios);
    }
    fclose(archivo);
    return config;
}

// Para comprobar si el usuario existe
bool verificarUsuario(const char *archivoLectura, int IdCuenta)
{
    FILE *archivoVerificar = fopen(archivoLectura, "r");
    if (archivoVerificar == NULL)
    {
        perror("Error al abrir archivo de cuentas");
        exit(1);
    }

    char linea[256];
    int id;

    while (fgets(linea, sizeof(linea), archivoVerificar))
    {
        sscanf(linea, "%d,", &id);
        if (id == IdCuenta)
        {
            fclose(archivoVerificar);
            return true;
        }
    }

    fclose(archivoVerificar);
    return false;
}

void *MostrarMonitor(void *arg)
{

    pid_t pidMonitor;
    pidMonitor = fork();
    if (pidMonitor == 0)
    {
        const char *rutaMonitor = configuracion.ruta_monitor;
        char comandoMonitor[512];
        snprintf(comandoMonitor, sizeof(comandoMonitor), "%s %d %d %s", rutaMonitor, configuracion.umbral_retiros, configuracion.umbral_transferencias, configuracion.archivo_transacciones);
        // Ejecutar gnome-terminal con el comando
        execlp("gnome-terminal", "gnome-terminal", "--", "bash", "-c", comandoMonitor, NULL);
    }
    else
    {
    }

    return NULL;
}

void *MostrarMenu(void *arg)
{
    // Zona para declarar variables
    int numeroCuenta;
    char datosUsuario[100];
    pid_t pidUsuario;      // El pidUsuario
    pid_t pidCrearUsuario; // Variable for creating user process

    Config configuracion = leer_configuracion("config.txt");

    // Inicializamos las cuentas
    InitCuentas(configuracion.archivo_cuentas);

    // Bucle de espera de conexiones

    do
    {
        printf("+-----------------------------+\n");
        printf("|    Bienvenido al Banco      |\n");
        printf("|  salir(1)                   |\n");
        printf("+-----------------------------+\n");
        printf("Introduce tu número de cuenta:\n");
        scanf("%d", &numeroCuenta);

        if (numeroCuenta == 1)
        {

            // Cierra la terminal que ejecutó el proceso (en la mayoría de casos)
            pid_t terminalPid = getppid();
            kill(terminalPid, SIGKILL);
            exit(EXIT_SUCCESS);
        }
        else
        {
            if (verificarUsuario(configuracion.archivo_cuentas, numeroCuenta))
            {
                printf("Nuevo usuario conectado. Iniciando sesión...\n");

                pidUsuario = fork();

                if (pidUsuario < 0)
                {
                    EscribirEnLog("Error al crear un usuario");
                    exit(EXIT_FAILURE);
                }

                if (pidUsuario == 0)
                { // Proceso hijo
                    // Ruta absoluta del ejecutable usuario
                    const char *rutaUsuario = configuracion.ruta_usuario;

                    // Construcción del comando con pausa al final
                    char comandoUsuario[512];
                    snprintf(comandoUsuario, sizeof(comandoUsuario), "\"%s\" %d \"%s\" \"%s\" \"%s\"; exit", rutaUsuario, numeroCuenta, configuracion.archivo_transacciones, configuracion.archivo_log, configuracion.archivo_cuentas);

                    // Ejecutar gnome-terminal con el comando
                    execlp("gnome-terminal", "gnome-terminal", "--", "bash", "-c", comandoUsuario, NULL);

                    // Si execlp falla
                    perror("Error al ejecutar gnome-terminal");
                    exit(EXIT_FAILURE);
                }
                else
                { // Proceso padre
                }
            }
            else
            {

                // Sirve para la creacion de Usuario
                pidCrearUsuario = fork();

                if (pidCrearUsuario < 0)
                {
                    EscribirEnLog("Error al iniciar el proceso de creación de usuario");
                    exit(EXIT_FAILURE);
                }

                if (pidCrearUsuario == 0)
                { // proceso hijo

                    // Ruta absoluta del ejecutable menu usuario
                    const char *rutaCrearUsuario = configuracion.ruta_crearusuario;

                    // Construcción del comando con pausa al final
                    char comandoCrearUsuario[512];
                    snprintf(comandoCrearUsuario, sizeof(comandoCrearUsuario), "%s %d %s", rutaCrearUsuario, numeroCuenta, configuracion.archivo_log);

                    // Ejecutar gnome-terminal con el comando
                    execlp("gnome-terminal", "gnome-terminal", "--", "bash", "-c", comandoCrearUsuario, NULL);
                }
                else
                {
                }
            }
        }

    } while (numeroCuenta != 1);
}

void *EscucharTuberiaMonitor(void *arg)
{
    int fdBancoMonitor;
    char mensaje[512];

    // Abrir la tubería FIFO para lectura
    fdBancoMonitor = open("fifo_bancoMonitor", O_RDONLY);
    if (fdBancoMonitor == -1)
    {
        EscribirEnLog("Error al abrir la tubería fifo_bancoMonitor");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        // Leer mensajes de la tubería
        int bytes_leidos = read(fdBancoMonitor, mensaje, sizeof(mensaje) - 1);
        if (bytes_leidos > 0)
        {
            mensaje[bytes_leidos] = '\0'; // Asegurar terminación de cadena
            printf("%s\n", mensaje);      // Mostrar el mensaje
        }
        else if (bytes_leidos == 0)
        {
            break; // Salir del bucle si no hay más datos
        }
    }

    close(fdBancoMonitor); // Cerrar la tubería
}

int main()
{

    pthread_t hilo_menu, hilo_pipes, hilo_monitor, hilo_escuchar;
    configuracion = leer_configuracion("config.txt");

    // Tuberias
    if (mkfifo("fifo_bancoMonitor", 0666) == -1 && errno != EEXIST)
    {
        EscribirEnLog("Error al crear la tubería");
        exit(EXIT_FAILURE);
    }

    // Crear los hilos
    pthread_create(&hilo_escuchar, NULL, EscucharTuberiaMonitor, NULL);
    pthread_create(&hilo_menu, NULL, MostrarMenu, NULL);
    pthread_create(&hilo_monitor, NULL, MostrarMonitor, NULL);

    pthread_join(hilo_menu, NULL);
    pthread_join(hilo_monitor, NULL);
    pthread_join(hilo_escuchar, NULL);
    return 0;
}