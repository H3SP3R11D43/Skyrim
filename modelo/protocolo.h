#ifndef PROTOCOLO_H
#define PROTOCOLO_H

#define MAX_PAYLOAD 8192
#define MAX_ACAO 32

int enviarPacote(int soquete, const char *acao, const char *payload);
int receberPacote(int soquete, char *acao, char *payload);

#endif
