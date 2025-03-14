#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    int limite_retiro;
    int limite_transferencia;
    int umbral_retiros;
    int umbral_transferencias;
    int num_hilos;
    char archivo_cuentas[50];
    char archivo_log[50];
} Config;

extern Config configuracion;  // Declaración de la variable global

Config leer_configuracion(const char *ruta);

#endif // CONFIG_H
