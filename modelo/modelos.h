#ifndef MODELOS_H
#define MODELOS_H

#define MAX_CIDADES 30
#define MAX_NOME 50
#define MAX_PASSAGEIROS 4
#define MAX_CARONAS 1000
#define MAX_RESERVAS 2000
#define MAX_ITINERARIOS 20

typedef enum {
    PASSAGEIRO,
    MOTORISTA
} TipoUsuario;

typedef struct {
    int socket;
    int id;
    char nome[100];
    char email[100];
    TipoUsuario tipo;
    int autenticado;
} Cliente;

typedef struct {
    char origem[MAX_NOME];
    char destino[MAX_NOME];
    float preco;
    int vagasTotal;
    int vagasDisponiveis;
    char passageirosEmails[MAX_PASSAGEIROS][100];
    int numPassageiros;
} Trecho;

typedef struct {
    int idCarona;
    char emailMotorista[100];
    char dataPartida[20];
    char horarioPartida[10];
    Trecho trechos[MAX_CIDADES - 1];
    int numTrechos;
    int ativa;
} Carona;

typedef struct {
    int idReserva;
    char emailPassageiro[100];
    int idCarona[MAX_CIDADES];
    int idxTrecho[MAX_CIDADES];
    int numTrechos;
    float precoTotal;
    int ativa;
} Reserva;

typedef struct {
    int idCarona[MAX_CIDADES];
    int idxTrecho[MAX_CIDADES];
    int numTrechos;
    float precoTotal;
} Itinerario;

void inicializarCarona(Carona *c, int idCarona, const char *emailMotorista, const char *data, const char *horario);
int adicionarTrecho(Carona *c, const char *origem, const char *destino, float preco, int vagas);
int tentarReservarTrecho(Trecho *t, const char *emailPassageiro);
void cancelarReservaTrecho(Trecho *t, const char *emailPassageiro);

#endif
