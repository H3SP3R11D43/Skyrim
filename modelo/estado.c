#include "estado.h"

pthread_mutex_t trava = PTHREAD_MUTEX_INITIALIZER;
 
Carona caronas[MAX_CARONAS];
int numCaronas = 0;
int proximoIdCarona = 1;
 
Reserva reservas[MAX_RESERVAS];
int numReservas = 0;
int proximoIdReserva = 1;

int contadorClientes = 0;

Carona *buscarCaronaPorId(int idCarona) {
    for (int i = 0; i < numCaronas; i++) {
        if (caronas[i].idCarona == idCarona) {
            return &caronas[i];
        }
    }
    return NULL;
}
