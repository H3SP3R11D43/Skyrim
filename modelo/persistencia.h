#ifndef PERSISTENCIA_H
#define PERSISTENCIA_H
 
#define ARQUIVO_USUARIOS "arquivos/usuarios.json"
#define ARQUIVO_CARONAS  "arquivos/caronas.json"
#define ARQUIVO_RESERVAS "arquivos/reservas.json"
 
int cadastrarUsuario(const char *payloadJSON);
int autenticar(const char *tipo, const char *email, const char *senha, char *nomeSaida);

void salvarCaronas(void);
void carregarCaronas(void);
 
void salvarReservas(void);
void carregarReservas(void);
 
#endif
