#ifndef ITINERARIOS_H
#define ITINERARIOS_H

#include "protocolo.h"
#include "modelos.h"

void tratarBuscarItinerario(const char *payload, Itinerario *cache, int *numCache,
char *respAcao, char *respPayload);
void tratarReservarItinerario(Cliente *cli, const char *payload, Itinerario *cache, int numCache,
char *respAcao, char *respPayload);
void tratarMinhasReservas(Cliente *cli, char *respAcao, char *respPayload);
void tratarCancelarReserva(Cliente *cli, const char *payload, char *respAcao, char *respPayload);

#endif
