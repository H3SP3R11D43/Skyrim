#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "caronas.h"
#include "estado.h"
#include "persistencia.h"
#include "modelos.h"

void tratarPublicarCarona(Cliente *cli, const char *payload, char *respAcao, char *respPayload) {
    char data[20] = "", horario[10] = "";
    char listaTrechos[MAX_PAYLOAD] = "";

    if (sscanf(payload, "%19[^|]|%9[^|]|%[^\n]", data, horario, listaTrechos) < 2) {
        strcpy(respAcao, "ERRO");
        strcpy(respPayload, "{\"status\": \"erro\", \"msg\": \"Payload de publicacao invalido\"}");
        return;
    }

    pthread_mutex_lock(&trava);
    if (numCaronas >= MAX_CARONAS) {
        pthread_mutex_unlock(&trava);

        strcpy(respAcao, "ERRO");
        strcpy(respPayload, "{\"status\": \"erro\", \"msg\": \"Capacidade maxima de caronas atingida\"}");
        return;
    }

    Carona *c = &caronas[numCaronas];
    int idNovo = proximoIdCarona++;
    inicializarCarona(c, idNovo, cli->email, data, horario);

    int trechosAdicionados = 0;
    char *item = strtok(listaTrechos, ";");

    while (item != NULL) {
        char origem[MAX_NOME] = "", destino[MAX_NOME] = "";
        float preco = 0; int vagas = 0;
        if (sscanf(item, "%49[^,],%49[^,],%f,%d", origem, destino, &preco, &vagas) == 4) {
            if (adicionarTrecho(c, origem, destino, preco, vagas)) trechosAdicionados++;
        }
        item = strtok(NULL, ";");
    }

    if (trechosAdicionados == 0) {
        pthread_mutex_unlock(&trava);
        strcpy(respAcao, "ERRO");
        strcpy(respPayload, "{\"status\": \"erro\", \"msg\": \"Nenhum trecho valido informado\"}");
        return;
    }

    numCaronas++;
    salvarCaronas();
    pthread_mutex_unlock(&trava);

    strcpy(respAcao, "RESPOSTA");
    snprintf(respPayload, MAX_PAYLOAD,
    "{\"status\": \"sucesso\", \"msg\": \"Carona publicada\", \"idCarona\": %d}", idNovo);
}

void tratarMinhasCaronas(Cliente *cli, char *respAcao, char *respPayload) {
    respPayload[0] = '\0';
    int encontrou = 0;

    pthread_mutex_lock(&trava);
    for (int i = 0; i < numCaronas; i++) {
        Carona *c = &caronas[i];

        if (strcmp(c->emailMotorista, cli->email) != 0) {
            continue;
        }
        encontrou = 1;

        char linha[1024];
        int off = snprintf(linha, sizeof(linha), "%d|%s|%s|%s",
        c->idCarona, c->dataPartida, c->horarioPartida,
        c->ativa ? "ATIVA" : "CANCELADA");

        for (int t = 0; t < c->numTrechos; t++) {
            Trecho *tr = &c->trechos[t];
            char passageiros[400] = "";

            for (int p = 0; p < tr->numPassageiros; p++) {
                strncat(passageiros, tr->passageirosEmails[p], sizeof(passageiros) - strlen(passageiros) - 2);
                if (p < tr->numPassageiros - 1) {
                    strncat(passageiros, ",", sizeof(passageiros) - strlen(passageiros) - 1);
                }
            }

            off += snprintf(linha + off, sizeof(linha) - off, ";%d:%s-%s:%.2f:%d/%d:%s",
            t, tr->origem, tr->destino, tr->preco,
            tr->vagasDisponiveis, tr->vagasTotal, passageiros);
        }
        strncat(respPayload, linha, MAX_PAYLOAD - strlen(respPayload) - 2);
        strncat(respPayload, "\n", MAX_PAYLOAD - strlen(respPayload) - 1);
    }
    pthread_mutex_unlock(&trava);

    strcpy(respAcao, "RESPOSTA");
    if (!encontrou) {
        strcpy(respPayload, "Nenhuma carona publicada.");
    }
}

void tratarCancelarCarona(Cliente *cli, const char *payload, char *respAcao, char *respPayload) {
    int idCarona = atoi(payload);

    pthread_mutex_lock(&trava);
    Carona *c = buscarCaronaPorId(idCarona);

    if (c == NULL || strcmp(c->emailMotorista, cli->email) != 0) {
        pthread_mutex_unlock(&trava);
        strcpy(respAcao, "ERRO");
        strcpy(respPayload, "{\"status\": \"erro\", \"msg\": \"Carona nao encontrada\"}");
        return;
    }

    c->ativa = 0;
    for (int i = 0; i < numReservas; i++) {
        Reserva *r = &reservas[i];
        if (!r->ativa) {
            continue;
        }

        for (int t = 0; t < r->numTrechos; t++) {
            if (r->idCarona[t] == idCarona) {
                r->ativa = 0;
            }
        }
    }
    
    salvarCaronas();
    salvarReservas();
    pthread_mutex_unlock(&trava);

    strcpy(respAcao, "RESPOSTA");
    strcpy(respPayload, "{\"status\": \"sucesso\", \"msg\": \"Carona cancelada\"}");
}
