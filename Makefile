# Simulador original em Raylib + ferramentas de terminal.
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS = -I.
LDLIBS = -lm
ifeq ($(OS),Windows_NT)
EXE = .exe
GUI_CFLAGS = -Ivendor/raylib/include
GUI_LIBS = vendor/raylib/lib/libraylib.a -lopengl32 -lgdi32 -lwinmm -mwindows
else
GUI_CFLAGS = $(shell pkg-config --cflags raylib)
GUI_LIBS = $(shell pkg-config --libs raylib)
endif

NUCLEO = memoria.c decodificador.c Aritimetica.c instrucoes_movimentacao_desvios.c sim_real.c
CABECALHOS = sicxe_config.h sicxe.h memoria.h decodificador.h Aritimetica.h instrucoes_movimentacao_desvios.h
MONTADOR = montagem/parser.c montagem/tabelas.c montagem/passos.c montagem/objeto.c
ASM_HEADERS = montagem/montador.h montagem/interno.h sicxe_config.h

.PHONY: all terminal teste executar interface clean
all: terminal interface
terminal: simulador$(EXE) montador$(EXE)
interface: simulador-gui$(EXE)

simulador-gui$(EXE): main.c memview.c memview.h programa.c programa.h $(NUCLEO) $(CABECALHOS) $(MONTADOR) $(ASM_HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(GUI_CFLAGS) -o $@ main.c memview.c programa.c $(NUCLEO) $(MONTADOR) $(GUI_LIBS) $(LDLIBS)

teste_programa$(EXE): testes/teste_programa.c programa.c programa.h $(NUCLEO) $(CABECALHOS) $(MONTADOR) $(ASM_HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ testes/teste_programa.c programa.c $(NUCLEO) $(MONTADOR) $(LDLIBS)

montador$(EXE): montagem/main.c $(MONTADOR) $(ASM_HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ montagem/main.c $(MONTADOR)

teste_montador$(EXE): testes/teste_montador.c $(MONTADOR) $(ASM_HEADERS) $(NUCLEO) $(CABECALHOS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ testes/teste_montador.c $(MONTADOR) $(NUCLEO) $(LDLIBS)

simulador$(EXE): main_cli.c $(NUCLEO) $(CABECALHOS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ main_cli.c $(NUCLEO) $(LDLIBS)

teste_sim_real$(EXE): teste_sim_real.c $(NUCLEO) $(CABECALHOS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ teste_sim_real.c $(NUCLEO) $(LDLIBS)

teste_integracao$(EXE): testes/teste_integracao.c $(NUCLEO) $(CABECALHOS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ testes/teste_integracao.c $(NUCLEO) $(LDLIBS)

teste_instrucoes$(EXE): testes/teste_instrucoes.c $(NUCLEO) $(CABECALHOS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ testes/teste_instrucoes.c $(NUCLEO) $(LDLIBS)

teste: teste_sim_real$(EXE) teste_integracao$(EXE) teste_instrucoes$(EXE) teste_montador$(EXE) teste_programa$(EXE)
	./teste_sim_real$(EXE) exemplo.obj
	./teste_integracao$(EXE)
	./teste_instrucoes$(EXE)
	./teste_montador$(EXE)
	./teste_programa$(EXE)

executar: simulador$(EXE)
	./simulador$(EXE) exemplo.obj

clean:
	rm -f simulador$(EXE) simulador-gui$(EXE) montador$(EXE) teste_programa$(EXE) teste_montador$(EXE) teste_sim_real$(EXE) teste_integracao$(EXE) teste_instrucoes$(EXE)
