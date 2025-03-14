#ifndef USUARIOS_H
#define USUARIOS_H

#include "Banco.h"

void realizar_deposito(int cuenta, float monto);
void realizar_retiro(int cuenta, float monto);
void realizar_transferencia(int origen, int destino, float monto);
void consultar_saldo(int cuenta);
void ejecutar_menu_usuario();

#endif //USUARIOS_H