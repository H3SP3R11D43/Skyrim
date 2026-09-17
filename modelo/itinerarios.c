#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "itinerarios.h"
#include "estado.h"
#include "persistencia.h"
#include "modelos.h"

static void dfsItinerario(const char *cidadeAtual, const char *destino, const char *data,
    Itinerario *atual, char visitados[][MAX_NOME], int numVisitados,
    Itinerario *resultados, int *numResultados) {

    if (*numResultados >= MAX_ITINERARIOS) return;
    if (atual->numTrechos >= MAX_CIDADES - 1) return;

    for (int i = 0; i < numCaronas && *numResultados < MAX_ITINERARIOS; i++) {
        Carona *c = &caronas[i];
        if (!c->ativa || strcmp(c->dataPartida, data) != 0) continue;

        for (int t = 0; t < c->numTrechos; t++) {
            Trecho *tr = &c->trechos[t];

            if (tr->vagasDisponiveis <= 0) continue;
            if (strcmp(tr->origem, cidadeAtual) != 0) continue;

            int jaVisitado = 0;
            for (int v = 0; v < numVisitados; v++) {
                if (strcmp(visitados[v], tr->destino) == 0) { jaVisitado = 1; break; }
            }
            if (jaVisitado) continue;

            Itinerario proximo = *atual;
            proximo.idCarona[proximo.numTrechos] = c->idCarona;
            proximo.idxTrecho[proximo.numTrechos] = t;
            proximo.numTrechos++;
            proximo.precoTotal += tr->preco;

            if (strcmp(tr->destino, destino) == 0) {
                resultados[*numResultados] = proximo;
                (*numResultados)++;

                if (*numResultados >= MAX_ITINERARIOS) return;
                continue;
            }

            char novosVisitados[MAX_CIDADES][MAX_NOME];
            memcpy(novosVisitados, visitados, sizeof(char) * MAX_NOME * numVisitados);
            strncpy(novosVisitados[numVisitados], tr->destino, MAX_NOME - 1);
            novosVisitados[numVisitados][MAX_NOME - 1] = '\0';

            dfsItinerario(tr->destino, destino, data, &proximo, novosVisitados, numVisitados + 1,
            resultados, numResultados);
        }
    }
}

void tratarBuscarItinerario(const char *payload, Itinerario *cache, int *numCache,
    char *respAcao, char *respPayload) {
    char origem[MAX_NOME] = "", destino[MAX_NOME] = "", data[20] = "";

    if (sscanf(payload, "%49[^|]|%49[^|]|%19s", origem, destino, data) != 3) {
        strcpy(respAcao, "ERRO");
        strcpy(respPayload, "{\"status\": \"erro\", \"msg\": \"Payload de busca invalido\"}");
        return;
    }

    Itinerario resultados[MAX_ITINERARIOS];
    int numResultados = 0;
    Itinerario vazio = {0};
    char visitados[MAX_CIDADES][MAX_NOME];
    strncpy(visitados[0], origem, MAX_NOME - 1);
    visitados[0][MAX_NOME - 1] = '\0';

    pthread_mutex_lock(&trava);
    dfsItinerario(origem, destino, data, &vazio, visitados, 1, resultados, &numResultados);
    pthread_mutex_unlock(&trava);

    memcpy(cache, resultados, sizeof(Itinerario) * numResultados);
    *numCache = numResultados;

    respPayload[0] = '\0';
    for (int i = 0; i < numResultados; i++) {
        char linha[1024];
        int off = snprintf(linha, sizeof(linha), "%d|%.2f", i, resultados[i].precoTotal);

        for (int t = 0; t < resultados[i].numTrechos; t++) {
            pthread_mutex_lock(&trava);

            Carona *c = buscarCaronaPorId(resultados[i].idCarona[t]);
            Trecho *tr = (c != NULL) ? &c->trechos[resultados[i].idxTrecho[t]] : NULL;

            if (tr != NULL) {
                off += snprintf(linha + off, sizeof(linha) - off, ";%d:%d:%s-%s",
                resultados[i].idCarona[t], resultados[i].idxTrecho[t], tr->origem, tr->destino);
            }
            pthread_mutex_unlock(&trava);
        }
        strncat(respPayload, linha, MAX_PAYLOAD - strlen(respPayload) - 2);
        strncat(respPayload, "\n", MAX_PAYLOAD - strlen(respPayload) - 1);
    }

    strcpy(respAcao, "RESPOSTA");
    if (numResultados == 0) strcpy(respPayload, "Nenhum itinerario encontrado.");
}

void tratarReservarItinerario(Cliente *cli, const char *payload, Itinerario *cache, int numCache,
char *respAcao, char *respPayload) {
    int idx = atoi(payload);

    if (idx < 0 || idx >= numCache) {
        strcpy(respAcao, "ERRO");
        strcpy(respPayload, "{\"status\": \"erro\", \"msg\": \"Itinerario invalido ou busca expirada\"}");
        return;
    }
    Itinerario *itin = &cache[idx];

    pthread_mutex_lock(&trava);

    for (int t = 0; t < itin->numTrechos; t++) {
        Carona *c = buscarCaronaPorId(itin->idCarona[t]);
        if (c == NULL || !c->ativa || c->trechos[itin->idxTrecho[t]].vagasDisponiveis <= 0) {
            pthread_mutex_unlock(&trava);
            strcpy(respAcao, "ERRO");
            strcpy(respPayload, "{\"status\": \"erro\", \"msg\": \"Um ou mais trechos ficaram indisponiveis, tente nova busca\"}");
            return;
        }
    }

    if (numReservas >= MAX_RESERVAS) {
        pthread_mutex_unlock(&trava);
        strcpy(respAcao, "ERRO");
        strcpy(respPayload, "{\"status\": \"erro\", \"msg\": \"Capacidade maxima de reservas atingida\"}");
        return;
    }

    Reserva *r = &reservas[numReservas++];
    memset(r, 0, sizeof(Reserva));
    r->idReserva = proximoIdReserva++;
    snprintf(r->emailPassageiro, sizeof(r->emailPassageiro), "%s", cli->email);
    r->numTrechos = itin->numTrechos;
    r->precoTotal = itin->precoTotal;
    r->ativa = 1;

    for (int t = 0; t < itin->numTrechos; t++) {
        Carona *c = buscarCaronaPorId(itin->idCarona[t]);
        Trecho *tr = &c->trechos[itin->idxTrecho[t]];
        tentarReservarTrecho(tr, cli->email);
        r->idCarona[t] = itin->idCarona[t];
        r->idxTrecho[t] = itin->idxTrecho[t];
    }

    salvarCaronas();
    salvarReservas();
    pthread_mutex_unlock(&trava);

    strcpy(respAcao, "RESPOSTA");
    snprintf(respPayload, MAX_PAYLOAD,
    "{\"status\": \"sucesso\", \"msg\": \"Itinerario reservado\", \"idReserva\": %d}", r->idReserva);
}

void tratarMinhasReservas(Cliente *cli, char *respAcao, char *respPayload) {
    respPayload[0] = '\0';
    int encontrou = 0;

    pthread_mutex_lock(&trava);
    for (int i = 0; i < numReservas; i++) {
        Reserva *r = &reservas[i];
        if (strcmp(r->emailPassageiro, cli->email) != 0) continue;
        encontrou = 1;

        char linha[512];
        int off = snprintf(linha, sizeof(linha), "%d|%.2f|%s",
                            r->idReserva, r->precoTotal, r->ativa ? "ATIVA" : "CANCELADA");
        for (int t = 0; t < r->numTrechos; t++) {
            Carona *c = buscarCaronaPorId(r->idCarona[t]);
            Trecho *tr = (c != NULL) ? &c->trechos[r->idxTrecho[t]] : NULL;
            if (tr != NULL) {
                off += snprintf(linha + off, sizeof(linha) - off, ";%s-%s", tr->origem, tr->destino);
            }
        }
        strncat(respPayload, linha, MAX_PAYLOAD - strlen(respPayload) - 2);
        strncat(respPayload, "\n", MAX_PAYLOAD - strlen(respPayload) - 1);
    }
    pthread_mutex_unlock(&trava);

    strcpy(respAcao, "RESPOSTA");
    if (!encontrou) strcpy(respPayload, "Nenhuma reserva encontrada.");
}

void tratarCancelarReserva(Cliente *cli, const char *payload, char *respAcao, char *respPayload) {
    int idReserva = atoi(payload);

    pthread_mutex_lock(&trava);
    Reserva *alvo = NULL;
    for (int i = 0; i < numReservas; i++) {
        if (reservas[i].idReserva == idReserva && strcmp(reservas[i].emailPassageiro, cli->email) == 0) {
            alvo = &reservas[i];
            break;
        }
    }
    if (alvo == NULL || !alvo->ativa) {
        pthread_mutex_unlock(&trava);
        strcpy(respAcao, "ERRO");
        strcpy(respPayload, "{\"status\": \"erro\", \"msg\": \"Reserva nao encontrada\"}");
        return;
    }

    for (int t = 0; t < alvo->numTrechos; t++) {
        Carona *c = buscarCaronaPorId(alvo->idCarona[t]);
        if (c != NULL) cancelarReservaTrecho(&c->trechos[alvo->idxTrecho[t]], cli->email);
    }
    alvo->ativa = 0;
    salvarCaronas();
    salvarReservas();
    pthread_mutex_unlock(&trava);

    strcpy(respAcao, "RESPOSTA");
    strcpy(respPayload, "{\"status\": \"sucesso\", \"msg\": \"Reserva cancelada\"}");
}
