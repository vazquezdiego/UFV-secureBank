#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int id;
    float saldo;
} Cuenta;

int main() {
    FILE *archivo = fopen("cuentas.dat", "wb");
    if (!archivo) {
        perror("Error al crear cuentas.dat");
        return 1;
    }
    
    Cuenta cuentas[3] = {{1, 1000.0}, {2, 2000.0}, {3, 3000.0}};
    fwrite(cuentas, sizeof(Cuenta), 3, archivo);
    fclose(archivo);
    
    printf("Cuentas inicializadas correctamente.\n");
    return 0;
}

