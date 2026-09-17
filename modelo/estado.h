#ifndef ESTADO_H
#define ESTADO_H
 
#include <pthread.h>

#include "modelos.h"

extern pthread_mutex_t trava;

extern Carona caronas[MAX_CARONAS];
extern int numCaronas;
extern int proximoIdCarona;
 
extern Reserva reservas[MAX_RESERVAS];
extern int numReservas;
extern int proximoIdReserva;

extern int contadorClientes;

Carona *buscarCaronaPorId(int idCarona);

#endif
