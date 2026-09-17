#ifndef CARONAS_H
#define CARONAS_H

#include "protocolo.h"
#include "modelos.h"

void tratarPublicarCarona(Cliente *cli, const char *payload, char *respAcao, char *respPayload);
void tratarMinhasCaronas(Cliente *cli, char *respAcao, char *respPayload);
void tratarCancelarCarona(Cliente *cli, const char *payload, char *respAcao, char *respPayload);

#endif
