//+---------------------------------------------------------------------------------------------------------------+
// Funciones:
//  - ejecutar_menu_usuario(int IdUsuario)
//  - buscarUsuarioEnArchivo(const char *rutaBuscar)
//+---------------------------------------------------------------------------------------------------------------+
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "Usuarios.h"
#include "Config.h"
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h> // Required for SIGKILL

// Definimos los semáforos para controlar el archivo de cuentas.txt cuando escribimos y leemos
sem_t sem1; // Para leer
sem_t sem2; // Para escribir

sem_t *semaforoTransacciones;

// Definimos un mutex para controlar las operaciones que realizan los usuarios
pthread_mutex_t mutex_u = PTHREAD_MUTEX_INITIALIZER;

typedef struct Cuenta
{
    int numero_cuenta;
    char titular[50];
    float saldo;
    int num_transacciones;

    // Definimos un mutex para controlar el acceso a la cuenta
    pthread_mutex_t mutex_c;
} Cuenta;

typedef struct
{
    Cuenta *usuario;
    const char *archivoTransacciones;
    const char *archivoCuentas;
} ArgsHilo;

void limpiarConsola()
{
    system("clear");
}

void ObtenerFechaHora(char *buffer, size_t bufferSize)
{
    time_t t;
    struct tm *tm_info;

    time(&t);
    tm_info = localtime(&t);

    strftime(buffer, bufferSize, "%Y-%m-%d %H:%M:%S", tm_info);
}

void EscribirEnLog(const char *mensaje, const char *archivoLog)
{
    FILE *archivo = fopen(archivoLog, "a"); // "a" → Añadir al final
    if (!archivo)
    {
        perror("Error al abrir el archivo de log");
        return;
    }

    fprintf(archivo, "%s", mensaje);

    fclose(archivo);
}

void EscribirEnTranscciones(const char *mensaje, const char *archivoTransacciones)
{
    sem_wait(semaforoTransacciones);                     // Esperar a que el semáforo esté disponible
    FILE *archivoLog = fopen(archivoTransacciones, "a"); // "a" → Añadir al final
    if (!archivoLog)
    {
        perror("Error al abrir el archivo de log");
        return;
    }

    // Escribir en el log con timestamp
    fprintf(archivoLog, "%s\n", mensaje);

    fclose(archivoLog);
    sem_post(semaforoTransacciones); // Liberar el semáforo
}

// Para extraer los campos del archivo
Cuenta LeerDatosUsuarioArchivo(const char *archivoLeer, int IdUsuario)
{
    // Pide permiso para leer el archivo
    sem_wait(&sem1);

    FILE *Puntero = fopen(archivoLeer, "r");
    if (Puntero == NULL)
    {
        perror("Error al abrir el archivo de usuarios");
        exit(1);
    }

    char LineaFichero[256];
    // Cuenta cuenta = {0, "", 0.0, 0}; // Inicializamos la cuenta

    Cuenta cuenta = {0, "", 0.0, 0, PTHREAD_MUTEX_INITIALIZER}; // Inicializamos la cuenta

    while (fgets(LineaFichero, sizeof(LineaFichero), Puntero) != NULL)
    {
        char *token;
        int numCuenta;
        float saldo;
        int numOperaciones;
        char titular[100];

        // Para extraer el número de cuenta
        token = strtok(LineaFichero, ",");
        if (token == NULL)
            continue;
        numCuenta = atoi(token);

        // Para extraer el titular
        token = strtok(NULL, ",");
        if (token == NULL)
            continue;
        strncpy(titular, token, sizeof(titular) - 1);
        titular[sizeof(titular) - 1] = '\0';

        // Para extraer el saldo
        token = strtok(NULL, ",");
        if (token == NULL)
            continue;
        saldo = atof(token);

        // Para extraer el número de operaciones
        token = strtok(NULL, ",");
        if (token == NULL)
            continue;
        numOperaciones = atoi(token);

        if (numCuenta == IdUsuario)
        {
            cuenta.numero_cuenta = numCuenta;
            strcpy(cuenta.titular, titular);
            cuenta.saldo = saldo;
            cuenta.num_transacciones = numOperaciones;

            // Inicializamos el mutex de la cuenta
            pthread_mutex_init(&cuenta.mutex_c, NULL);
            break;
        }
    }

    fclose(Puntero);

    // Liberamos el semáforo para que otros hilos puedan acceder al archivo
    sem_post(&sem1);
    return cuenta;
}

// Guarda los cambios en el archivo
void GuardarCuentaEnArchivo(const char *archivo, Cuenta usuario)
{
    // Pide permiso para escribir en el archivo
    sem_wait(&sem2);

    // Abrimos el archivo en modo escritura y lectura
    FILE *Puntero = fopen(archivo, "r+");
    if (Puntero == NULL)
    {
        perror("Error al abrir el archivo para escritura");
        exit(1);
    }

    char lineas[100][256]; // almacena todas las líneas del archivo
    int totalLineas = 0;
    char buffer[256];

    // Lee todas las líneas del archivo
    while (fgets(buffer, sizeof(buffer), Puntero))
    {
        strcpy(lineas[totalLineas], buffer);
        totalLineas++;
    }

    rewind(Puntero); // Vuelve al inicio para escribir

    // escribe los nuevos datos en el archivo
    for (int i = 0; i < totalLineas; i++)
    {
        int id;
        sscanf(lineas[i], "%d,", &id);
        if (id == usuario.numero_cuenta)
        {
            fprintf(Puntero, "%d,%s,%.2f,%d\n", usuario.numero_cuenta, usuario.titular, usuario.saldo, usuario.num_transacciones);
        }
        else
        {
            fputs(lineas[i], Puntero);
        }
    }

    fclose(Puntero);

    // Liberamos el semáforo para que otros hilos puedan acceder al archivo
    sem_post(&sem2);
}

// Depositar dinero en la cuenta, usándo semáforos para los hilos y guardando las modificaciones en el archivo
void *Depositar(void *arg)
{
    // Bloqueamos el mutex para evitar condiciones de carrera
    pthread_mutex_lock(&mutex_u);

    ArgsHilo *args = (ArgsHilo *)arg;

    // Bloqueamos el mutex de la cuenta
    pthread_mutex_lock(&args->usuario->mutex_c);

    Cuenta *usuario = args->usuario;
    float cantidad;
    char FechaHora[20]; // Para almacenar la fecha y la hora
    char mensaje[256];

    printf("Ingrese la cantidad que quiere depositar: ");
    if (scanf("%f", &cantidad) != 1 || cantidad <= 0)
    {
        printf("Cantidad inválida\n");
        // Desbloqueamos los mutex
        pthread_mutex_unlock(&args->usuario->mutex_c);
        pthread_mutex_unlock(&mutex_u);
        return NULL;
    }

    // Sumamos la cantidad al saldo, y suma el número de transacciones
    usuario->saldo += cantidad;
    usuario->num_transacciones++;

    // Guardamos los cambios en el archivo
    GuardarCuentaEnArchivo(args->archivoCuentas, *usuario);

    // escribimos el mensaje
    ObtenerFechaHora(FechaHora, sizeof(FechaHora));
    snprintf(mensaje, sizeof(mensaje), "[%s] Depósito en cuenta %d: +%.2f \n", FechaHora, usuario->numero_cuenta, cantidad);
    EscribirEnLog(mensaje, args->archivoTransacciones);

    printf("Deposito realizado con éxito. Nuevo saldo: %.2f\n", usuario->saldo);

    pthread_mutex_unlock(&args->usuario->mutex_c);
    pthread_mutex_unlock(&mutex_u);
    sleep(5);
    limpiarConsola();
    return NULL;
}

// Retirar dinero de la cuenta, usándo semáforos para los hilos y guardando las modificaciones en el archivo
void *Retirar(void *arg)
{
    // Bloqueamos el mutex para evitar condiciones de carrera
    pthread_mutex_lock(&mutex_u);

    ArgsHilo *args = (ArgsHilo *)arg;

    // Bloqueamos el mutex de la cuenta
    pthread_mutex_lock(&args->usuario->mutex_c);

    Cuenta *usuario = args->usuario;
    float cantidad;
    char FechaHora[20]; // Para almacenar la fecha y la hora
    char mensaje[256];

    printf("Ingrese la cantidad a retirar: ");
    if (scanf("%f", &cantidad) != 1 || cantidad <= 0)
    {
        printf("Cantidad inválida\n");

        // Desbloqueamos los mutex
        pthread_mutex_unlock(&args->usuario->mutex_c);
        pthread_mutex_unlock(&mutex_u);

        return NULL;
    }

    // Restamos la cantidad al saldo, y suma el número de transacciones
    if (usuario->saldo >= cantidad)
    {
        usuario->saldo -= cantidad;
        usuario->num_transacciones++;
        GuardarCuentaEnArchivo(args->archivoCuentas, *usuario);

        // escribimos el mensaje
        ObtenerFechaHora(FechaHora, sizeof(FechaHora));
        snprintf(mensaje, sizeof(mensaje), "[%s] Retiro en cuenta %d: -%.2f \n", FechaHora, usuario->numero_cuenta, cantidad);
        EscribirEnLog(mensaje, args->archivoTransacciones);

        printf("Retiro realizado con éxito. Nuevo saldo: %.2f\n", usuario->saldo);
    }
    else
    {
        printf("Fondos insuficientes\n");
    }

    // Desbloqueamos los mutex
    pthread_mutex_unlock(&args->usuario->mutex_c);
    pthread_mutex_unlock(&mutex_u);
    sleep(5);
    limpiarConsola();

    return NULL;
}

// Consultar el saldo de la cuenta
void *ConsultarSaldo(void *arg)
{
    // Bloqueamos el mutex para evitar condiciones de carrera
    pthread_mutex_lock(&mutex_u);

    ArgsHilo *args = (ArgsHilo *)arg;

    // Bloqueamos el mutex de la cuenta
    pthread_mutex_lock(&args->usuario->mutex_c);

    Cuenta *usuario = args->usuario;
    char FechaHora[20]; // Para almacenar la fecha y la hora
    char mensaje[256];

    printf("Saldo actual: %.2f\n", usuario->saldo);

    // Desbloqueamos los mutex
    pthread_mutex_unlock(&args->usuario->mutex_c);
    pthread_mutex_unlock(&mutex_u);
    sleep(5);
    limpiarConsola();
    return NULL;
}

// Realizar transacción a otra cuenta
void *Transferencia(void *arg)
{
    // Bloqueamos el mutex para evitar condiciones de carrera
    pthread_mutex_lock(&mutex_u);

    ArgsHilo *args = (ArgsHilo *)arg;

    // Bloqueamos el mutex de la cuenta
    pthread_mutex_lock(&args->usuario->mutex_c);

    Cuenta *usuario = args->usuario;
    int cuentaDestino;
    float cantidad;
    char FechaHora[20]; // Para almacenar la fecha y la hora
    char mensaje[256];

    // Leemos los datos de la cuenta destino y la cantidad a transferir
    printf("Ingrese el número de cuenta destino: ");
    if (scanf("%d", &cuentaDestino) != 1)
    {
        printf("Número de cuenta inválido\n");

        // Desbloqueamos los mutex
        pthread_mutex_unlock(&args->usuario->mutex_c);
        pthread_mutex_unlock(&mutex_u);

        return NULL;
    }

    // Leemos la cantidad a transferir
    printf("Ingrese la cantidad a transferir: ");
    if (scanf("%f", &cantidad) != 1 || cantidad <= 0)
    {
        printf("Cantidad inválida\n");

        // Desbloqueamos los mutex
        pthread_mutex_unlock(&args->usuario->mutex_c);
        pthread_mutex_unlock(&mutex_u);

        return NULL;
    }

    // Leemos los datos de la cuenta destino
    Cuenta destino = LeerDatosUsuarioArchivo(args->archivoCuentas, cuentaDestino);
    if (destino.numero_cuenta == 0)
    {
        printf("Cuenta destino no encontrada\n");

        // Liberamos el semáforo para que otros hilos puedan acceder al archivo
        sem_post(&sem2);
        sem_post(&sem1);

        // Desbloqueamos los mutex
        pthread_mutex_unlock(&args->usuario->mutex_c);
        pthread_mutex_unlock(&mutex_u);

        return NULL;
    }

    // Realizamos la transacción
    if (usuario->saldo >= cantidad)
    {
        usuario->saldo -= cantidad;
        destino.saldo += cantidad;
        usuario->num_transacciones++;
        destino.num_transacciones++;
        GuardarCuentaEnArchivo(args->archivoCuentas, *usuario);
        GuardarCuentaEnArchivo(args->archivoCuentas, destino);

        // escribimos el mensaje
        ObtenerFechaHora(FechaHora, sizeof(FechaHora));
        snprintf(mensaje, sizeof(mensaje), "[%s] Transferencia desde cuenta %d a cuenta %d: -%.2f \n", FechaHora, usuario->numero_cuenta, cuentaDestino, cantidad);
        EscribirEnLog(mensaje, args->archivoTransacciones);

        printf("Transacción realizada con éxito. Nuevo saldo: %.2f\n", usuario->saldo);
    }
    else
    {
        printf("Fondos insuficientes\n");
    }

    // Liberamos el semáforo para que otros hilos puedan acceder al archivo
    sem_post(&sem2);
    sem_post(&sem1);

    // Desbloqueamos los mutex
    pthread_mutex_unlock(&args->usuario->mutex_c);
    pthread_mutex_unlock(&mutex_u);
    sleep(5);
    limpiarConsola();
    return NULL;
}

void *MostrarMenuUsuario()
{
    printf("+--------------------------------------+\n");
    printf("|             MENÚ USUARIO             |\n");
    printf("|             1. Depositar             |\n");
    printf("|             2. Retirar               |\n");
    printf("|             3. Consultar saldo       |\n");
    printf("|             4. Transferencia         |\n");
    printf("|             5. Salir                 |\n");
    printf("+--------------------------------------+\n");
    printf("\nSeleccione una opción: ");
}

// Para conseguir el pid de la terminal actual
pid_t get_terminal_pid()
{
    return getppid(); // devuelve el ppid del padre
}

int main(int argc, char *argv[])
{
    // Inicializamos los semáforos y el mutex de usuario
    sem_init(&sem1, 0, 1);
    sem_init(&sem2, 0, 1);

    pthread_mutex_init(&mutex_u, NULL);

    int numeroCuenta = atoi(argv[1]);
    char *archivoTransacciones = argv[2];
    const char *archivoLog = argv[3];
    const char *archivoCuentas = argv[4];

    char FechaInicioCuenta[148];
    char MensajeDeInicio[256];
    Cuenta usuario = LeerDatosUsuarioArchivo("cuentas.txt", numeroCuenta);

    ObtenerFechaHora(FechaInicioCuenta, sizeof(FechaInicioCuenta));
    snprintf(MensajeDeInicio, sizeof(MensajeDeInicio), "[%s] Inicio de sesión de cuenta: %s\n", FechaInicioCuenta, argv[1]);
    EscribirEnLog(MensajeDeInicio, archivoLog);

    printf("Bienvenido, %s (Cuenta: %d)\n", usuario.titular, usuario.numero_cuenta);

    int opcion;
    pthread_t hilo;

    do
    {
        MostrarMenuUsuario();

        if (scanf("%d", &opcion) != 1)
        {
            printf("Opción inválida\n");
            while (getchar() != '\n')
                ; // Limpiar buffer
            continue;
        }

        ArgsHilo args = {&usuario, archivoTransacciones, archivoCuentas};

        // Con las opciones creamos los hilos, y hacemos que esperen a que terminen
        switch (opcion)
        {
        case 1:
            pthread_create(&hilo, NULL, Depositar, &args);
            pthread_join(hilo, NULL);
            break;
        case 2:
            pthread_create(&hilo, NULL, Retirar, &args);
            pthread_join(hilo, NULL);
            break;
        case 3:
            pthread_create(&hilo, NULL, ConsultarSaldo, &args);
            pthread_join(hilo, NULL);
            break;
        case 4:
            pthread_create(&hilo, NULL, Transferencia, &args);
            pthread_join(hilo, NULL);
            break;
        case 5:
            printf("Cerrando la cuenta...\n");
            char FechaFinCuenta[148];
            char MensajeDeSalida[256];
            ObtenerFechaHora(FechaFinCuenta, sizeof(FechaFinCuenta));
            snprintf(MensajeDeSalida, sizeof(MensajeDeSalida), "[%s] Cierre de sesión de cuenta: %d\n", FechaFinCuenta, usuario.numero_cuenta);
            EscribirEnLog(MensajeDeSalida, archivoLog);
            pthread_mutex_destroy(&mutex_u);
            sem_destroy(&sem1);
            sem_destroy(&sem2);

            // Cierra la terminal que ejecutó el proceso (en la mayoría de casos)
            pid_t terminalPid = getppid();
            kill(terminalPid, SIGKILL);
            exit(EXIT_SUCCESS);

        default:
            printf("Opción no válida\n");
        }
    } while (opcion != 5);

    pthread_mutex_destroy(&mutex_u);
    pthread_mutex_destroy(&usuario.mutex_c);
    sem_destroy(&sem1);
    sem_destroy(&sem2);

    return EXIT_SUCCESS;
}
