#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#include "modelo/modelos.h"
#include "modelo/protocolo.h"
#include "modelo/estado.h"
#include "modelo/persistencia.h"
#include "modelo/caronas.h"
#include "modelo/itinerarios.h"

#define PORTA 8080

void *tratarCliente(void *arg) {
    Cliente *cliente = (Cliente *) arg;
    int soquete = cliente->socket;
    int id = cliente->id;

    char acao[MAX_ACAO] = {0};
    char payload[MAX_PAYLOAD] = {0};
    char respAcao[MAX_ACAO], respPayload[MAX_PAYLOAD];

    Itinerario cacheItinerarios[MAX_ITINERARIOS];
    int numCacheItinerarios = 0;

    while (receberPacote(soquete, acao, payload)) {
        if (strcmp(acao, "SAIR") == 0) break;

        if (!cliente->autenticado) {
            if (strcmp(acao, "CADASTRO") == 0) {
                if (cadastrarUsuario(payload)) {
                    enviarPacote(soquete, "RESPOSTA", "{\"status\": \"sucesso\", \"msg\": \"Cadastro realizado\"}");
                    printf("Cliente [%d] realizou cadastro.\n", id);
                } else {
                    enviarPacote(soquete, "ERRO", "{\"status\": \"erro\", \"msg\": \"Falha ao gravar cadastro\"}");
                }

            } else if (strcmp(acao, "LOGIN") == 0) {
                char tipo[20] = "", email[100] = "", senha[100] = "", nome[100] = "";
                sscanf(payload, "{\"tipo\": \"%19[^\"]\", \"email\": \"%99[^\"]\", \"senha\": \"%99[^\"]\"}",
                tipo, email, senha);

                if (autenticar(tipo, email, senha, nome)) {
                    cliente->autenticado = 1;
                    cliente->tipo = (strcmp(tipo, "MOTORISTA") == 0) ? MOTORISTA : PASSAGEIRO;
                    snprintf(cliente->email, sizeof(cliente->email), "%s", email);
                    snprintf(cliente->nome, sizeof(cliente->nome), "%s", nome);

                    char resp[256];
                    snprintf(resp, sizeof(resp), "{\"status\": \"sucesso\", \"msg\": \"Login efetuado\", \"nome\": \"%s\"}", nome);
                    enviarPacote(soquete, "RESPOSTA", resp);
                    printf("Cliente [%d] (%s) efetuou login.\n", id, email);
                } else {
                    enviarPacote(soquete, "ERRO", "{\"status\": \"erro\", \"msg\": \"Credenciais invalidas\"}");
                }

            } else {
                enviarPacote(soquete, "ERRO", "{\"status\": \"erro\", \"msg\": \"Faca login ou cadastro primeiro\"}");
            }
            continue;
        }

        if (cliente->tipo == MOTORISTA && strcmp(acao, "PUBLICAR_CARONA") == 0) {
            tratarPublicarCarona(cliente, payload, respAcao, respPayload);
        } else if (cliente->tipo == MOTORISTA && strcmp(acao, "MINHAS_CARONAS") == 0) {
            tratarMinhasCaronas(cliente, respAcao, respPayload);
        } else if (cliente->tipo == MOTORISTA && strcmp(acao, "CANCELAR_CARONA") == 0) {
            tratarCancelarCarona(cliente, payload, respAcao, respPayload);
        } else if (cliente->tipo == PASSAGEIRO && strcmp(acao, "BUSCAR_ITINERARIO") == 0) {
            tratarBuscarItinerario(payload, cacheItinerarios, &numCacheItinerarios, respAcao, respPayload);
        } else if (cliente->tipo == PASSAGEIRO && strcmp(acao, "RESERVAR_ITINERARIO") == 0) {
            tratarReservarItinerario(cliente, payload, cacheItinerarios, numCacheItinerarios, respAcao, respPayload);
        } else if (cliente->tipo == PASSAGEIRO && strcmp(acao, "MINHAS_RESERVAS") == 0) {
            tratarMinhasReservas(cliente, respAcao, respPayload);
        } else if (cliente->tipo == PASSAGEIRO && strcmp(acao, "CANCELAR_RESERVA") == 0) {
            tratarCancelarReserva(cliente, payload, respAcao, respPayload);
        } else {
            strcpy(respAcao, "ERRO");
            strcpy(respPayload, "{\"status\": \"erro\", \"msg\": \"Acao desconhecida ou nao permitida para esse tipo de usuario\"}");
        }

        printf("Cliente [%d] requisitou %s -> %s\n", id, acao, respAcao);
        enviarPacote(soquete, respAcao, respPayload);
    }

    printf("Cliente [%d] desconectou.\n", id);
    close(soquete);
    free(cliente);
    return NULL;
}

int main() {
    signal(SIGPIPE, SIG_IGN);

    carregarCaronas();
    carregarReservas();

    int servidorFD = socket(AF_INET, SOCK_STREAM, 0), opt = 1;
    setsockopt(servidorFD, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    struct sockaddr_in endereco;
    endereco.sin_family = AF_INET;
    endereco.sin_addr.s_addr = INADDR_ANY;
    endereco.sin_port = htons(PORTA);

    if (bind(servidorFD, (struct sockaddr*) &endereco, sizeof(endereco)) < 0) {
        perror("bind");
        return 1;
    }
    listen(servidorFD, 50);

    printf("Servidor VaiJunto online na porta %d. Esperando clientes...\n", PORTA);

    while (1) {
        struct sockaddr_in enderecoCliente;
        socklen_t tamanhoEndereco = sizeof(enderecoCliente);

        int soqueteCliente = accept(servidorFD, (struct sockaddr*) &enderecoCliente, &tamanhoEndereco);
        if (soqueteCliente < 0) continue;

        Cliente *novoCliente = malloc(sizeof(Cliente));
        memset(novoCliente, 0, sizeof(Cliente));
        novoCliente->socket = soqueteCliente;

        pthread_mutex_lock(&trava);
        novoCliente->id = ++contadorClientes;
        pthread_mutex_unlock(&trava);

        pthread_t threadID;
        pthread_create(&threadID, NULL, tratarCliente, (void *) novoCliente);
        pthread_detach(threadID);
    }

    close(servidorFD);
    return 0;
}
