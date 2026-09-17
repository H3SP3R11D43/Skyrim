#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "modelos.h"

void inicializarCarona(Carona *carona, int idCarona, const char *emailMotorista, const char *data, const char *horario) {
    memset(carona, 0, sizeof(Carona));

    carona->idCarona = idCarona;
    strncpy(carona->emailMotorista, emailMotorista, sizeof(carona->emailMotorista) - 1);
    strncpy(carona->dataPartida, data, sizeof(carona->dataPartida) - 1);
    strncpy(carona->horarioPartida, horario, sizeof(carona->horarioPartida) - 1);
    carona->numTrechos = 0;
    carona->ativa = 1;
}

int adicionarTrecho(Carona *carona, const char *origem, const char *destino, float preco, int vagas) {
    if (carona->numTrechos >= MAX_CIDADES - 1) {
        return 0;
    }

    if (vagas > MAX_PASSAGEIROS) {
        vagas = MAX_PASSAGEIROS;
    }

    Trecho *trecho = &carona->trechos[carona->numTrechos];
    memset(trecho, 0, sizeof(Trecho));
    strncpy(trecho->origem, origem, MAX_NOME - 1);
    strncpy(trecho->destino, destino, MAX_NOME - 1);

    trecho->preco = preco;
    trecho->vagasTotal = vagas;
    trecho->vagasDisponiveis = vagas;
    trecho->numPassageiros = 0;

    carona->numTrechos++;
    return 1;
}

int tentarReservarTrecho(Trecho *trecho, const char *emailPassageiro) {
    if (trecho->vagasDisponiveis <= 0) {
        return 0;
    }

    if (trecho->numPassageiros >= MAX_PASSAGEIROS) {
        return 0;
    }

    strncpy(trecho->passageirosEmails[trecho->numPassageiros], emailPassageiro,
            sizeof(trecho->passageirosEmails[trecho->numPassageiros]) - 1);
    trecho->passageirosEmails[trecho->numPassageiros][sizeof(trecho->passageirosEmails[0]) - 1] = '\0';

    trecho->numPassageiros++;
    trecho->vagasDisponiveis--;
    return 1;
}

void cancelarReservaTrecho(Trecho *trecho, const char *emailPassageiro) {
    for (int i = 0; i < trecho->numPassageiros; i++) {
        if (strcmp(trecho->passageirosEmails[i], emailPassageiro) == 0) {
            strcpy(trecho->passageirosEmails[i], trecho->passageirosEmails[trecho->numPassageiros - 1]);
            trecho->numPassageiros--;
            trecho->vagasDisponiveis++;
            return;
        }
    }
}
