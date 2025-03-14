#include <stdio.h>
#include <time.h>

void write_log(const char *message) {
    FILE *logFile = fopen("program.log", "a");  // "a" → Añadir al final
    if (!logFile) {
        perror("Error al abrir el archivo log");
        return;
    }

    // Obtener la hora actual
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    // Escribir en el log con timestamp
    fprintf(logFile, "[%04d-%02d-%02d %02d:%02d:%02d] %s\n",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec, message);

    fclose(logFile);
}

int main() {
    write_log("El programa ha iniciado.");
    write_log("Se ejecutó una operación importante.");
    write_log("El programa ha finalizado correctamente.");

    printf("Mensajes guardados en program.log\n");
    return 0;
}
