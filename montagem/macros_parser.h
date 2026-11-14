#ifndef MACROS_PARSER_H
#define MACROS_PARSER_H
#include <stddef.h>

#define MACRO_PARSER_TAM_LINHA       512
#define MACRO_PARSER_TAM_NOME        32
#define MACRO_PARSER_TAM_OPERANDO    256
#define MACRO_PARSER_MAX_PARAMETROS  32

typedef enum {
    MACRO_LINHA_VAZIA = 0,
    MACRO_LINHA_COMENTARIO,
    MACRO_LINHA_NORMAL,
    MACRO_LINHA_DEFINICAO_V1,
    MACRO_LINHA_DELIMITADOR_V2,
    MACRO_LINHA_FIM_DEFINICAO
} MacroTipoLinha;

typedef enum {
    MACRO_SINTAXE_V1 = 1,
    MACRO_SINTAXE_V2 = 2
} MacroSintaxe;

typedef struct {
    char nome[MACRO_PARSER_TAM_NOME];
    char valorPadrao[MACRO_PARSER_TAM_OPERANDO];
    int palavraChave;
    int temPadrao;
} MacroParametroFormal;

typedef struct {
    char nome[MACRO_PARSER_TAM_NOME];
    MacroParametroFormal parametros[MACRO_PARSER_MAX_PARAMETROS];
    size_t quantidadeParametros;
    MacroSintaxe sintaxe;
    int possuiParametroRotulo;
} MacroPrototipo;

typedef struct {
    char nome[MACRO_PARSER_TAM_NOME];
    char valor[MACRO_PARSER_TAM_OPERANDO];
    int palavraChave;
} MacroArgumento;

typedef struct {
    char nome[MACRO_PARSER_TAM_NOME];
    MacroArgumento argumentos[MACRO_PARSER_MAX_PARAMETROS];
    size_t quantidadeArgumentos;
    MacroSintaxe sintaxe;
} MacroChamada;

typedef struct {
    MacroTipoLinha tipo;
    char original[MACRO_PARSER_TAM_LINHA];
    char codigo[MACRO_PARSER_TAM_LINHA];
    char comentario[MACRO_PARSER_TAM_LINHA];
    MacroPrototipo prototipo; /* preenchido quando a definicao for V1 */
} MacroLinhaInfo;

typedef struct {
    char rotulo[MACRO_PARSER_TAM_NOME];
    char operacao[MACRO_PARSER_TAM_NOME];
    char operando[MACRO_PARSER_TAM_OPERANDO];
    char comentario[MACRO_PARSER_TAM_LINHA];
    int temRotulo;
} MacroCamposLinha;

/* O controlador fornece uma funcao que reconhece instrucoes, diretivas e macros. */
typedef int (*MacroReconhecerOperacao)(const char *nome, void *contexto);

/* Classifica linha normal, comentario, MCDEFN, MCEND ou definicao V1. */
int macro_analisar_linha(const char *linha, MacroLinhaInfo *resultado, char *erro, size_t tamanhoErro);

/* Analisa o prototipo que aparece depois de MCDEFN isolado (versao 2). */
int macro_analisar_prototipo_v2(const char *linha, MacroPrototipo *resultado, char *erro, size_t tamanhoErro);

/* Separa rotulo, operacao e operando. O callback evita duplicar a MOT/POT. */
int macro_separar_campos(const MacroLinhaInfo *linha, MacroReconhecerOperacao reconhecerOperacao, void *contexto, MacroCamposLinha *campos, char *erro, size_t tamanhoErro);

/* 1 = chamada reconhecida; 0 = outra linha; -1 = chamada com sintaxe invalida. */
int macro_analisar_chamada(const char *linha, const MacroPrototipo *prototipo, MacroChamada *resultado, char *erro, size_t tamanhoErro);

int macro_identificador_valido(const char *texto);

#endif
