#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "Usuarios.h"
#include "Config.h"
#include "crearUsuario.h"
#include <unistd.h>


// Para crear un usuario nuevo
int main (int argc, char *argv[]) {

    // Obtener los valores pasados desde el programa principal
    int NumeroCuenta = atoi(argv[1]);
    char Titular[100];
    float Saldo = 0;
    int NumeroOperaciones = 0;

    printf("+-----------------------------+\n");
    printf("| Menu de creación de usuario |\n");
    printf("+-----------------------------+\n");

    printf("Ingrese Titular: ");
    scanf(" %s", Titular);
    printf("+------------------------------------------------------+\n");
    printf("|   Su saldo y numero de transacciones empezara en 0.  |\n");


    // Guardar en archivo cuentas.txt
    FILE *archivo = fopen("cuentas.txt", "a");
    if (archivo == NULL) {
        perror("Error al abrir el archivo");
        return EXIT_FAILURE;
    }

    fprintf(archivo, "%d, %s, %.2f, %d\n", NumeroCuenta, Titular, Saldo, NumeroOperaciones);
    fclose(archivo);

    printf("|   Usuario guardado con éxito.                        |\n");
    printf("+------------------------------------------------------+\n");


}


    /*FILE *archivo = fopen(archivoEscritura, "a");
    if (archivo == NULL) {
        perror("Error al abrir el archivo de cuentas");
        return;
    }

    fprintf(archivo, "%d,%s,0.00,0\n", IdCuenta, Nombre);
    fclose(archivo);*/
