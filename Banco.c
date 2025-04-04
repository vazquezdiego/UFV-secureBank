// +------------------------------------------------------------------------------------------------------------------------------------------------+/
//  El programa principal, con ejecutar este programa vale para el resto
//
//
// Funciones que contiene el archivo:
// - Config leer_configuracion(const char *ruta): guardar la configuración en un struct
// - bool verificarUsuario(const char *archivoLectura, int IdCuenta): devuelve true or false dependiendo si existe o no
//   hay que revisarlo porque no funciona
// - void crearUsuario(const char *archivoEscritura, const char *Nombre, int IdCuenta): para crear el usuario en caso de que no exista
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

#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#include "Usuarios.h"
#include "Banco.h"
#include "init_cuentas.c"
#include "crearUsuario.h"
#include "Config.h"
#include "Monitor.h"

Config configuracion;

// funcion para escribir en el log
void EscribirEnTranscciones(const char *mensaje)
{
    FILE *archivoLog = fopen(configuracion.archivo_transacciones, "a"); // "a" → Añadir al final
    if (!archivoLog)
    {
        perror("Error al abrir el archivo de log");
        return;
    }


    // Escribir en el log con timestamp
    fprintf(archivoLog, "%s\n", mensaje);

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

void *VerPipes(void *arg)
{

    int fdBancoUsuario;


    while (1) // Bucle externo para volver a abrir el FIFO si se cierra
    {
        fdBancoUsuario = open("fifo_bancoUsuario", O_RDONLY);
        if (fdBancoUsuario == -1)
        {
            perror("Error al abrir la tubería.");
            sleep(1); // Espera un segundo antes de intentar abrir nuevamente
            continue;
        }

        while (1) // Bucle interno para leer mensajes continuamente
        {
            char mensaje[256] = {0};                                               // Asegurar que el buffer está limpio
            int bytes_leidos = read(fdBancoUsuario, mensaje, sizeof(mensaje) - 1); // Dejar espacio para '\0'

            if (bytes_leidos > 0)
            {
                mensaje[bytes_leidos] = '\0'; // Asegurar terminación de cadena
                EscribirEnTranscciones(mensaje);       // Escribir en el log

            }
            else if (bytes_leidos == 0)
            {
                // FIFO cerrada, salir del bucle interno y reabrirla
                close(fdBancoUsuario);
                break;
            }
        }
    }
}

void *MostrarMonitor(void *arg)
{

    pid_t pidMonitor;
    pidMonitor = fork();
    if (pidMonitor == 0)
    {
        const char *rutaMonitor = "/home/vboxuser/Documents/UFV-secureBank/monitor";
        char comandoMonitor[512];
        snprintf(comandoMonitor, sizeof(comandoMonitor), "%s %d %d", rutaMonitor, configuracion.limite_retiro, configuracion.limite_transferencia);
        // Ejecutar gnome-terminal con el comando
        execlp("gnome-terminal", "gnome-terminal", "--", "bash", "-c", comandoMonitor, NULL);
    }
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
    while (numeroCuenta != 1)
    {

        printf("+-----------------------------+\n");
        printf("|    Bienvenido al Banco      |\n");
        printf("|  salir(1)                   |\n");
        printf("+-----------------------------+\n");
        printf("Introduce tu número de cuenta:\n");
        scanf("%d", &numeroCuenta);

        if (verificarUsuario(configuracion.archivo_cuentas, numeroCuenta))
        {
            printf("Nuevo usuario conectado. Iniciando sesión...\n");
            sleep(1);

            pidUsuario = fork();

            if (pidUsuario < 0)
            {
                perror("Error al crear el proceso hijo");
                exit(EXIT_FAILURE);
            }

            if (pidUsuario == 0)
            { // Proceso hijo

                // Ruta absoluta del ejecutable usuario
                const char *rutaUsuario = "/home/vboxuser/Documents/UFV-secureBank/usuario";

                // Construcción del comando con pausa al final
                char comandoUsuario[512];
                snprintf(comandoUsuario, sizeof(comandoUsuario), "%s %d", rutaUsuario, numeroCuenta);

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

            if (pidCrearUsuario == 0)
            { // proceso hijo

                // Ruta absoluta del ejecutable menu usuario
                const char *rutaCrearUsuario = "/home/vboxuser/Documents/UFV-secureBank/usuario";

                // Construcción del comando con pausa al final
                char comandoCrearUsuario[512];
                snprintf(comandoCrearUsuario, sizeof(comandoCrearUsuario), "%s %d", rutaCrearUsuario, numeroCuenta);

                // Ejecutar gnome-terminal con el comando
                execlp("gnome-terminal", "gnome-terminal", "--", "bash", "-c", comandoCrearUsuario, NULL);
            }
            else
            {
            }
        }
    }
}

int main()
{
    pthread_t hilo_menu, hilo_pipes, hilo_monitor;
    configuracion = leer_configuracion("config.txt");
    // Tuberias
    if (mkfifo("fifo_bancoUsuario", 0666) == -1 && errno != EEXIST)
    {
        perror("Error al crear la tubería");
        exit(EXIT_FAILURE);
    }

    pthread_create(&hilo_menu, NULL, MostrarMenu, NULL);
    pthread_create(&hilo_pipes, NULL, VerPipes, NULL);
    pthread_create(&hilo_monitor, NULL, MostrarMonitor, NULL);

    pthread_join(hilo_menu, NULL);
    pthread_join(hilo_pipes, NULL);
    pthread_join(hilo_monitor, NULL);

    return 0;
}