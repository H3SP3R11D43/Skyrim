#include <stdio.h>
#include <string.h>

#include "controle_autenticacao.h"
#include "../modelo/protocolo.h"

extern int enviarEReceber(int servidorFD, const char *acao, const char *payload,
                           char *respAcao, char *respPayload);

static int campoVazio(const char *texto) {
    if (texto == NULL) return 1;
    for (const char *p = texto; *p != '\0'; p++) {
        if (*p != ' ') return 0;
    }
    return 1;
}

RespostaAutenticacao controleLogin(int servidorFD, const char *tipo,
                                    const char *email, const char *senha) {
    RespostaAutenticacao resultado = {0};

    if (campoVazio(email) || campoVazio(senha)) {
        resultado.codigo = AUTH_CAMPOS_VAZIOS;
        snprintf(resultado.mensagem, sizeof(resultado.mensagem),
                 "Preencha email e senha para entrar.");
        return resultado;
    }

    char payload[MAX_PAYLOAD];
    char respAcao[MAX_ACAO];
    char respPayload[MAX_PAYLOAD];

    snprintf(payload, sizeof(payload),
             "{\"tipo\": \"%s\", \"email\": \"%s\", \"senha\": \"%s\"}",
             tipo, email, senha);

    if (!enviarEReceber(servidorFD, "LOGIN", payload, respAcao, respPayload)) {
        resultado.codigo = AUTH_FALHA_SERVIDOR;
        snprintf(resultado.mensagem, sizeof(resultado.mensagem), "%s", respPayload);
        return resultado;
    }

    resultado.codigo = (strstr(respPayload, "sucesso") != NULL)
                            ? AUTH_OK
                            : AUTH_CREDENCIAIS_INVALIDAS;
    snprintf(resultado.mensagem, sizeof(resultado.mensagem), "%s", respPayload);
    return resultado;
}

RespostaAutenticacao controleCadastro(int servidorFD, const char *tipo,
                                       const char *nome, const char *contato,
                                       const char *email, const char *senha) {
    RespostaAutenticacao resultado = {0};

    if (campoVazio(nome) || campoVazio(contato) || campoVazio(email) || campoVazio(senha)) {
        resultado.codigo = AUTH_CAMPOS_VAZIOS;
        snprintf(resultado.mensagem, sizeof(resultado.mensagem),
                 "Preencha nome, contato, email e senha para cadastrar.");
        return resultado;
    }

    char payload[MAX_PAYLOAD];
    char respAcao[MAX_ACAO];
    char respPayload[MAX_PAYLOAD];

    snprintf(payload, sizeof(payload),
             "{\"tipo\": \"%s\", \"nome\": \"%s\", \"contato\": \"%s\", \"email\": \"%s\", \"senha\": \"%s\"}",
             tipo, nome, contato, email, senha);

    if (!enviarEReceber(servidorFD, "CADASTRO", payload, respAcao, respPayload)) {
        resultado.codigo = AUTH_FALHA_SERVIDOR;
        snprintf(resultado.mensagem, sizeof(resultado.mensagem), "%s", respPayload);
        return resultado;
    }

    resultado.codigo = (strstr(respPayload, "sucesso") != NULL)
                            ? AUTH_OK
                            : AUTH_FALHA_SERVIDOR;
    snprintf(resultado.mensagem, sizeof(resultado.mensagem), "%s", respPayload);
    return resultado;
}
