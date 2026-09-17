CC = gcc
CFLAGS = -Wall -std=c11 -D_DEFAULT_SOURCE

RAYLIB_DIR = raylib/src
RAYLIB_LIB = $(RAYLIB_DIR)/libraylib.a
RAYLIB_INCLUDE = -I$(RAYLIB_DIR)

RAYLIB_LIBS = -lm -lpthread -ldl -lrt -lX11

COMUM = modelo/modelos.c modelo/protocolo.c

SERVIDOR_SRC = servidor.c modelo/estado.c modelo/persistencia.c \
               modelo/caronas.c modelo/itinerarios.c $(COMUM)
CLIENTE_SRC  = cliente.c modelo/protocolo.c

GUI_SRC      = main.c modelo/protocolo.c \
               interface/mapa.c interface/telaLogin.c \
               interface/painelMotorista.c interface/painelPassageiro.c \
               interface/tema.c \
               controle/controle_autenticacao.c \
               controle/controle_caronas.c \
               controle/controle_itinerarios.c

all: servidor cliente oblivion

servidor: $(SERVIDOR_SRC)
	$(CC) $(CFLAGS) -o servidor $(SERVIDOR_SRC) -lpthread

cliente: $(CLIENTE_SRC)
	$(CC) $(CFLAGS) -o cliente $(CLIENTE_SRC)

oblivion: $(GUI_SRC) $(RAYLIB_LIB)
	$(CC) $(CFLAGS) -o oblivion $(GUI_SRC) $(RAYLIB_INCLUDE) $(RAYLIB_LIB) $(RAYLIB_LIBS)

$(RAYLIB_LIB):
	$(MAKE) -C $(RAYLIB_DIR) PLATFORM=PLATFORM_DESKTOP

clean:
	rm -f servidor cliente oblivion

.PHONY: all clean
