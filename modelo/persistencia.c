#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "persistencia.h"
#include "estado.h"
#include "modelos.h"
#include "protocolo.h"

int cadastrarUsuario(const char *payloadJSON) {
    pthread_mutex_lock(&trava);

    FILE *arquivo = fopen(ARQUIVO_USUARIOS, "a");
    if (arquivo == NULL) {
        pthread_mutex_unlock(&trava);
        return 0;
    }
    
    fprintf(arquivo, "%s\n", payloadJSON);
    fclose(arquivo);
    
    pthread_mutex_unlock(&trava);
    return 1;
}

int autenticar(const char *tipo, const char *email, const char *senha, char *nomeSaida) {
    int autenticado = 0;
    pthread_mutex_lock(&trava);

    char buscaTipo[40], buscaEmail[150], buscaSenha[150];
    snprintf(buscaTipo, sizeof(buscaTipo), "\"tipo\": \"%s\"", tipo);
    snprintf(buscaEmail, sizeof(buscaEmail), "\"email\": \"%s\"", email);
    snprintf(buscaSenha, sizeof(buscaSenha), "\"senha\": \"%s\"", senha);

    FILE *arquivo = fopen(ARQUIVO_USUARIOS, "r");
    if (arquivo != NULL) {
        char linha[MAX_PAYLOAD];
        while (fgets(linha, sizeof(linha), arquivo)) {
            if (strstr(linha, buscaTipo) && strstr(linha, buscaEmail) && strstr(linha, buscaSenha)) {
                autenticado = 1;
                char nome[100] = "";
                sscanf(strstr(linha, "\"nome\""), "\"nome\": \"%99[^\"]\"", nome);
                snprintf(nomeSaida, 100, "%s", nome);
                break;
            }
        }
        fclose(arquivo);
    }

    pthread_mutex_unlock(&trava);
    return autenticado;
}

void salvarCaronas(void) {
    FILE *f = fopen(ARQUIVO_CARONAS, "w");
    if (f == NULL) {
        return;
    }

    for (int i = 0; i < numCaronas; i++) {
        Carona *c = &caronas[i];
        char trechosBuf[MAX_PAYLOAD] = "";

        for (int t = 0; t < c->numTrechos; t++) {
            Trecho *tr = &c->trechos[t];
            char emails[500] = "";

            for (int p = 0; p < tr->numPassageiros; p++) {
                strncat(emails, tr->passageirosEmails[p], sizeof(emails) - strlen(emails) - 2);
                if (p < tr->numPassageiros - 1) {
                    strncat(emails, "&", sizeof(emails) - strlen(emails) - 1);
                }
            }
            
            char item[700];
            snprintf(item, sizeof(item), "%s,%s,%.2f,%d,%d,%s;",
            tr->origem, tr->destino, tr->preco, tr->vagasTotal, tr->vagasDisponiveis, emails);
            strncat(trechosBuf, item, sizeof(trechosBuf) - strlen(trechosBuf) - 1);
        }

        fprintf(f, "%d|%s|%s|%s|%d|%s\n",
        c->idCarona, c->dataPartida, c->horarioPartida, c->emailMotorista, c->ativa, trechosBuf);
    }
    fclose(f);
}

void carregarCaronas(void) {
    FILE *f = fopen(ARQUIVO_CARONAS, "r");
    if (f == NULL) {
        return;
    }

    char linha[MAX_PAYLOAD];
    while (fgets(linha, sizeof(linha), f) && numCaronas < MAX_CARONAS) {
        linha[strcspn(linha, "\n")] = '\0';

        if (linha[0] == '\0') {
            continue;
        }

        Carona *c = &caronas[numCaronas];
        memset(c, 0, sizeof(Carona));

        char trechosBuf[MAX_PAYLOAD] = "";
        int ativa = 1;
        if (sscanf(linha, "%d|%19[^|]|%9[^|]|%99[^|]|%d|%[^\n]",
        &c->idCarona, c->dataPartida, c->horarioPartida, c->emailMotorista, &ativa, trechosBuf) < 5) {
            continue;
        }
        c->ativa = ativa;
        char *item = strtok(trechosBuf, ";");

        while (item != NULL && c->numTrechos < MAX_CIDADES - 1) {
            Trecho *tr = &c->trechos[c->numTrechos];
            char emails[500] = "";

            if (sscanf(item, "%49[^,],%49[^,],%f,%d,%d,%[^\n]",
            tr->origem, tr->destino, &tr->preco, &tr->vagasTotal, &tr->vagasDisponiveis, emails) >= 5) {
                char *email = strtok(emails, "&");

                while (email != NULL && tr->numPassageiros < MAX_PASSAGEIROS) {
                    strncpy(tr->passageirosEmails[tr->numPassageiros], email, 99);
                    tr->numPassageiros++;
                    email = strtok(NULL, "&");
                }
                c->numTrechos++;
            }
            item = strtok(NULL, ";");
        }

        if (c->idCarona >= proximoIdCarona) proximoIdCarona = c->idCarona + 1;
        numCaronas++;
    }
    fclose(f);
    printf("Carregadas %d caronas de %s.\n", numCaronas, ARQUIVO_CARONAS);
}

void salvarReservas(void) {
    FILE *f = fopen(ARQUIVO_RESERVAS, "w");
    if (f == NULL) {
        return;
    }

    for (int i = 0; i < numReservas; i++) {
        Reserva *r = &reservas[i];
        char itens[512] = "";

        for (int t = 0; t < r->numTrechos; t++) {
            char item[40];
            snprintf(item, sizeof(item), "%d:%d,", r->idCarona[t], r->idxTrecho[t]);
            strncat(itens, item, sizeof(itens) - strlen(itens) - 1);
        }

        fprintf(f, "%d|%s|%.2f|%d|%s\n",
        r->idReserva, r->emailPassageiro, r->precoTotal, r->ativa, itens);
    }
    fclose(f);
}

void carregarReservas(void) {
    FILE *f = fopen(ARQUIVO_RESERVAS, "r");
    if (f == NULL) {
        return;
    }

    char linha[1024];
    while (fgets(linha, sizeof(linha), f) && numReservas < MAX_RESERVAS) {
        linha[strcspn(linha, "\n")] = '\0';
        if (linha[0] == '\0') continue;

        Reserva *r = &reservas[numReservas];
        memset(r, 0, sizeof(Reserva));
        char itens[512] = "";
        int ativa = 1;

        if (sscanf(linha, "%d|%99[^|]|%f|%d|%[^\n]",
        &r->idReserva, r->emailPassageiro, &r->precoTotal, &ativa, itens) < 4) {
            continue;
        }
        r->ativa = ativa;

        char *item = strtok(itens, ",");

        while (item != NULL && r->numTrechos < MAX_CIDADES) {
            int idCarona = 0, idxTrecho = 0;
            if (sscanf(item, "%d:%d", &idCarona, &idxTrecho) == 2) {
                r->idCarona[r->numTrechos] = idCarona;
                r->idxTrecho[r->numTrechos] = idxTrecho;
                r->numTrechos++;
            }
            item = strtok(NULL, ",");
        }

        if (r->idReserva >= proximoIdReserva) proximoIdReserva = r->idReserva + 1;
        numReservas++;
    }
    fclose(f);
    printf("Carregadas %d reservas de %s.\n", numReservas, ARQUIVO_RESERVAS);
}
