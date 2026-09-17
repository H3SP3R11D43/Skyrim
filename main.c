#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "raylib.h"
#include "modelo/protocolo.h"

#include "interface/contexto.h"
#include "interface/telaLogin.h"
#include "interface/tema.h"
#include "interface/mapa.h"
#include "interface/painelMotorista.h"
#include "interface/painelPassageiro.h"

#define PORTA 8080

static int conectarServidor(const char *host) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    struct sockaddr_in endereco;
    endereco.sin_family = AF_INET;
    endereco.sin_port = htons(PORTA);
    if (inet_pton(AF_INET, host, &endereco.sin_addr) <= 0) { close(fd); return -1; }
    if (connect(fd, (struct sockaddr *)&endereco, sizeof(endereco)) < 0) { close(fd); return -1; }
    return fd;
}

int enviarEReceber(int servidorFD, const char *acao, const char *payload, char *respAcao, char *respPayload) {
    if (servidorFD < 0) {
        strcpy(respAcao, "ERRO"); strcpy(respPayload, "Sem conexao.");
        return 0;
    }
    enviarPacote(servidorFD, acao, payload);
    if (!receberPacote(servidorFD, respAcao, respPayload)) {
        strcpy(respAcao, "ERRO"); strcpy(respPayload, "Conexao perdida."); return 0;
    }
    return strcmp(respAcao, "RESPOSTA") == 0;
}

void atualizarCampoTexto(ContextoApp *ctx, CampoTexto *campo) {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(GetMousePosition(), campo->caixa)) ctx->campoAtivo = campo;
        else if (ctx->campoAtivo == campo) ctx->campoAtivo = NULL;
    }
    if (ctx->campoAtivo != campo) return;
    int tecla = GetCharPressed();
    while (tecla > 0) {
        int len = (int)strlen(campo->texto);
        if (tecla >= 32 && tecla <= 125 && len < (int)sizeof(campo->texto) - 1) {
            campo->texto[len] = (char)tecla; campo->texto[len + 1] = '\0';
        }
        tecla = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
        int len = (int)strlen(campo->texto); if (len > 0) campo->texto[len - 1] = '\0';
    }
    if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_ENTER)) ctx->campoAtivo = NULL;
}

int main(int argc, char *argv[]) {
    const char *host = (argc > 1) ? argv[1] : (getenv("VAIJUNTO_SERVIDOR") ? getenv("VAIJUNTO_SERVIDOR") : "127.0.0.1");

    ContextoApp ctx = {0};
    ctx.servidorFD = conectarServidor(host);
    ctx.origemIndex = -1; ctx.destinoIndex = -1;
    ctx.origemTrechoIndex = -1; ctx.destinoTrechoIndex = -1;
    ctx.telaAtual = TELA_LOGIN;
    strcpy(ctx.mensagemStatus, ctx.servidorFD >= 0 ? "" : "Erro: Servidor Indisponivel");

    int painelVisivel = 1;
    const float LARGURA_PAINEL = 380.0f;
    const float LARGURA_ABA = 26.0f;
    const float ALTURA_ABA = 90.0f;

    int largura = 1000, altura = 700;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI);
    InitWindow(largura, altura, "OBLIVION: Vai com Deus");
    SetWindowPosition((GetMonitorWidth(0) - largura) / 2, (GetMonitorHeight(0) - altura) / 2);
    SetTargetFPS(60);

    carregarRecursosSkyrim();
    carregarMapa();

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

        int larguraAtual = GetScreenWidth();
        int alturaAtual = GetScreenHeight();

        if (ctx.telaAtual == TELA_LOGIN || ctx.telaAtual == TELA_CADASTRO) {
            DesenharTelaLoginECadastro(&ctx, larguraAtual, alturaAtual);
            continue;
        }

        float larguraPainel = painelVisivel ? LARGURA_PAINEL : 0.0f;
        float larguraMapa = larguraAtual - larguraPainel;

        // Posicao da aba: encostada na borda do painel quando visivel,
        // ou grudada na borda direita da tela quando o painel esta escondido.
        Rectangle rAba = {
            painelVisivel ? larguraMapa - LARGURA_ABA : larguraAtual - LARGURA_ABA,
            (alturaAtual - ALTURA_ABA) / 2.0f,
            LARGURA_ABA,
            ALTURA_ABA
        };

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), rAba)) {
            painelVisivel = !painelVisivel;
        }

        BeginDrawing();
        ClearBackground(BLACK);

        // 1. Atualiza e Renderiza o Mapa na área esquerda (ocupa a tela toda se o painel estiver escondido)
        atualizarEDesenharMapa(&ctx, larguraMapa, alturaAtual);

        // 2. Renderiza o Fundo do Painel Lateral na área direita
        if (painelVisivel) {
            Rectangle painel = { larguraMapa, 0, larguraPainel, (float)alturaAtual };
            DrawRectangleRec(painel, (Color){10, 10, 12, 235});
            DrawLineEx((Vector2){larguraMapa, 0}, (Vector2){larguraMapa, (float)alturaAtual}, 2, WHITE);

            // 3. Renderiza o Painel da Tela Ativa
            if (ctx.telaAtual == TELA_PASSAGEIRO) {
                DesenharPainelPassageiro(&ctx, painel);
            } else if (ctx.telaAtual == TELA_MOTORISTA) {
                DesenharPainelMotorista(&ctx, painel, alturaAtual);
            }
        }

        // 3b. Aba de esconder/mostrar o painel, sempre desenhada por cima
        DrawRectangleRec(rAba, (Color){10, 10, 12, 235});
        DrawRectangleLinesEx(rAba, 2, WHITE);
        const char *simboloAba = painelVisivel ? ">" : "<";
        int twAba = MedirTextoSkyrim(simboloAba, 20);
        DrawTextSkyrim(simboloAba, (int)(rAba.x + rAba.width / 2 - twAba / 2), (int)(rAba.y + rAba.height / 2 - 10), 20, WHITE);

        // 4. Barra de Status Inferior no Mapa
        DrawRectangle(0, alturaAtual - 30, (int)larguraMapa, 30, (Color){0, 0, 0, 200});
        DrawTextSkyrim(ctx.mensagemStatus, 10, alturaAtual - 24, 16, WHITE);

        EndDrawing();
    }

    if (ctx.servidorFD >= 0) { enviarPacote(ctx.servidorFD, "SAIR", ""); close(ctx.servidorFD); }
    descarregarMapa();
    descarregarRecursosSkyrim();
    CloseWindow();
    return 0;
}
