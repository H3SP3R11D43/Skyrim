#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "modelo/protocolo.h"

#define PORTA 8080

static void limparBufferEntrada(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

static void lerLinha(const char *rotulo, char *destino, int tamanho) {
    printf("%s", rotulo);
    if (fgets(destino, tamanho, stdin) == NULL) {
        destino[0] = '\0';
        return;
    }
    destino[strcspn(destino, "\n")] = 0;
}

static int lerInteiroSeguro(const char *mensagem, int *destino) {
    while (1) {
        printf("%s", mensagem);
        int ret = scanf("%d", destino);
        if (ret == 1) {
            limparBufferEntrada();
            return 1;
        }
        if (ret == EOF) return 0;
        printf("Erro: Digite um numero inteiro valido.\n");
        limparBufferEntrada();
    }
}

static int lerFloatSeguro(const char *mensagem, float *destino) {
    while (1) {
        printf("%s", mensagem);
        int ret = scanf("%f", destino);
        if (ret == 1) {
            limparBufferEntrada();
            return 1;
        }
        if (ret == EOF) return 0;
        printf("Erro: Digite um valor numerico valido.\n");
        limparBufferEntrada();
    }
}

static void painelMotorista(int fd) {
    char acao[MAX_ACAO];
    static char payload[MAX_PAYLOAD + 256];

    while (1) {
        printf("\n--- Painel do Motorista ---\n");
        printf("1. Publicar carona\n2. Minhas caronas\n3. Cancelar carona\n0. Sair\n");
        int opcao = 0;
        lerInteiroSeguro(">> ", &opcao);

        if (opcao == 0) { enviarPacote(fd, "SAIR", ""); break; }

        if (opcao == 1) {
            char data[20], horario[10];
            lerLinha("Data (ex: 2026-09-20): ", data, sizeof(data));
            lerLinha("Horario (ex: 08:00): ", horario, sizeof(horario));

            char trechos[MAX_PAYLOAD] = "";
            int numTrechos = 0;
            lerInteiroSeguro("Quantos trechos essa rota tem? ", &numTrechos);

            for (int i = 0; i < numTrechos; i++) {
                char origem[50], destino[50]; float preco = 0; int vagas = 0;
                printf("\nTrecho %d:\n", i + 1);
                lerLinha("  Origem: ", origem, sizeof(origem));
                lerLinha("  Destino: ", destino, sizeof(destino));
                lerFloatSeguro("  Preco: ", &preco);
                lerInteiroSeguro("  Vagas: ", &vagas);

                char item[256];
                snprintf(item, sizeof(item), "%s,%s,%.2f,%d;", origem, destino, preco, vagas);
                strncat(trechos, item, sizeof(trechos) - strlen(trechos) - 1);
            }

            snprintf(payload, sizeof(payload), "%s|%s|%s", data, horario, trechos);
            enviarPacote(fd, "PUBLICAR_CARONA", payload);
        } else if (opcao == 2) {
            enviarPacote(fd, "MINHAS_CARONAS", "");
        } else if (opcao == 3) {
            char idCarona[20];
            lerLinha("ID da carona a cancelar: ", idCarona, sizeof(idCarona));
            enviarPacote(fd, "CANCELAR_CARONA", idCarona);
        } else {
            continue;
        }

        if (receberPacote(fd, acao, payload)) {
            printf("\nServidor [%s]:\n%s\n", acao, payload);
        }
    }
}

static void painelPassageiro(int fd) {
    char acao[MAX_ACAO], payload[MAX_PAYLOAD];

    while (1) {
        printf("\n--- Painel do Passageiro ---\n");
        printf("1. Buscar itinerario\n2. Reservar itinerario (da ultima busca)\n3. Minhas reservas\n4. Cancelar reserva\n0. Sair\n");
        int opcao = 0;
        lerInteiroSeguro(">> ", &opcao);

        if (opcao == 0) { enviarPacote(fd, "SAIR", ""); break; }

        if (opcao == 1) {
            char origem[50], destino[50], data[20];
            lerLinha("Origem: ", origem, sizeof(origem));
            lerLinha("Destino: ", destino, sizeof(destino));
            lerLinha("Data (ex: 2026-09-20): ", data, sizeof(data));

            snprintf(payload, sizeof(payload), "%s|%s|%s", origem, destino, data);
            enviarPacote(fd, "BUSCAR_ITINERARIO", payload);
        } else if (opcao == 2) {
            char idItin[20];
            lerLinha("ID do itinerario (mostrado na busca): ", idItin, sizeof(idItin));
            enviarPacote(fd, "RESERVAR_ITINERARIO", idItin);
        } else if (opcao == 3) {
            enviarPacote(fd, "MINHAS_RESERVAS", "");
        } else if (opcao == 4) {
            char idReserva[20];
            lerLinha("ID da reserva a cancelar: ", idReserva, sizeof(idReserva));
            enviarPacote(fd, "CANCELAR_RESERVA", idReserva);
        } else {
            continue;
        }

        if (receberPacote(fd, acao, payload)) {
            printf("\nServidor [%s]:\n%s\n", acao, payload);
        }
    }
}

int main(int argc, char *argv[]) {
    const char *host = "127.0.0.1";
    if (argc > 1) host = argv[1];       /* uso: ./cliente <ip_do_servidor> */
    else if (getenv("VAIJUNTO_SERVIDOR")) host = getenv("VAIJUNTO_SERVIDOR");

    int clienteFD = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in enderecoServidor;
    enderecoServidor.sin_family = AF_INET;
    enderecoServidor.sin_port = htons(PORTA);
    if (inet_pton(AF_INET, host, &enderecoServidor.sin_addr) <= 0) {
        printf("Endereco de servidor invalido: %s\n", host);
        return -1;
    }

    if (connect(clienteFD, (struct sockaddr*) &enderecoServidor, sizeof(enderecoServidor)) < 0) {
        printf("Falha na conexao com %s:%d.\n", host, PORTA);
        return -1;
    }

    int opcaoTipo = 0, opcaoAcao = 0;
    printf("VAI COM DEUS - conectado em %s\n", host);
    printf("1. Passageiro\n2. Motorista\n");
    lerInteiroSeguro("O que voce e: ", &opcaoTipo);

    char tipo[20];
    strcpy(tipo, (opcaoTipo == 2) ? "MOTORISTA" : "PASSAGEIRO");

    printf("\n1. Cadastro\n2. Login\n");
    lerInteiroSeguro(" >> ", &opcaoAcao);

    char nome[100], contato[100], email[100], senha[100];
    char payloadJSON[MAX_PAYLOAD];

    if (opcaoAcao == 1) {
        lerLinha("Nome: ", nome, sizeof(nome));
        lerLinha("Contato: ", contato, sizeof(contato));
        lerLinha("Email: ", email, sizeof(email));
        lerLinha("Senha: ", senha, sizeof(senha));

        snprintf(payloadJSON, sizeof(payloadJSON),
                "{\"tipo\": \"%s\", \"nome\": \"%s\", \"contato\": \"%s\", \"email\": \"%s\", \"senha\": \"%s\"}",
                tipo, nome, contato, email, senha);

        enviarPacote(clienteFD, "CADASTRO", payloadJSON);
    } else {
        lerLinha("Email: ", email, sizeof(email));
        lerLinha("Senha: ", senha, sizeof(senha));

        snprintf(payloadJSON, sizeof(payloadJSON),
                "{\"tipo\": \"%s\", \"email\": \"%s\", \"senha\": \"%s\"}",
                tipo, email, senha);

        enviarPacote(clienteFD, "LOGIN", payloadJSON);
    }

    char acaoResposta[MAX_ACAO];
    char payloadResposta[MAX_PAYLOAD];

    if (receberPacote(clienteFD, acaoResposta, payloadResposta)) {
        printf("\nServidor [%s]: %s\n", acaoResposta, payloadResposta);

        if (opcaoAcao == 2 && strstr(payloadResposta, "sucesso") != NULL) {
            if (opcaoTipo == 2) painelMotorista(clienteFD);
            else painelPassageiro(clienteFD);
        }
    }

    close(clienteFD);
    printf("Voce desconectou.\n");
    return 0;
}
