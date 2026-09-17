#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#include "protocolo.h"

int enviarPacote(int soquete, const char *acao, const char*payload) {
    int tamanhoPayload = (int) strlen(payload);
    int tamanhoTotal = tamanhoPayload + 128;

    char *pacote = malloc(tamanhoTotal);
    if (pacote == NULL) {
        return -1;
    }

    int tamanhoCabecalho = snprintf(pacote, tamanhoTotal, "ACAO: %s\r\nTAMANHO: %d\r\n\r\n", acao, tamanhoPayload);

    memcpy(pacote + tamanhoCabecalho, payload, tamanhoPayload);
    int tamanhoFinal = tamanhoCabecalho + tamanhoPayload;

    int enviados = 0;
    while (enviados < tamanhoFinal) {
        int n = send(soquete, pacote + enviados, tamanhoFinal - enviados, 0);
        if (n <= 0) { 
            free(pacote);
            return -1;
        }
        enviados += n;
    }

    free(pacote);
    return tamanhoFinal;
}

int receberPacote(int soquete, char *acao, char *payload) {
    char cabecalho[256] = {0};
    int lidosCabecalho = 0;

    while (lidosCabecalho < (int) sizeof(cabecalho) - 1) {
        char c;
        int n = read(soquete, &c, 1);

        if (n <= 0) {
            return 0;
        }

        cabecalho[lidosCabecalho++] = c;
        if (lidosCabecalho >= 4 && strcmp(cabecalho + lidosCabecalho - 4, "\r\n\r\n") == 0) {
            break;
        }
    }

    int tamanhoEsperado = 0;
    if (sscanf(cabecalho, "ACAO: %31s\r\nTAMANHO: %d", acao, &tamanhoEsperado) != 2) {
        return 0;
    }
    if (tamanhoEsperado < 0 || tamanhoEsperado >= MAX_PAYLOAD) {
        return 0;
    }

    int payloadRecebido = 0;
    while (payloadRecebido < tamanhoEsperado) {
        int n = read(soquete, payload + payloadRecebido, tamanhoEsperado - payloadRecebido);
        if (n <= 0) return 0;
        payloadRecebido += n;
    }
    payload[payloadRecebido] = '\0';

    return 1;
}
