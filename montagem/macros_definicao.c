#include "macros_interno.h"
#include "macros_parser.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#ifndef MAX_NOME
#define MAX_NOME    32   // Tamanho maximo do nome do macro ou parametro
#endif
#ifndef MAX_PARAMS
#define MAX_PARAMS  16   // Quantidade maxima de parametros por macro
#endif
#ifndef MAX_LINHAS
#define MAX_LINHAS  256  // Quantidade maxima de linhas dentro do corpo de uma macro
#endif
#ifndef TAM_LINHA
#define TAM_LINHA   512  // Tamanho maximo de caracteres em uma linha
#endif
#ifndef MAX_NIVEL
#define MAX_NIVEL   (MC_NIVEIS + 1) // Profundidade maxima de macros definidas dentro de outras
#endif

#ifndef MODO_NORMAL
#define MODO_NORMAL 0    // Fora de macro: apenas copia instrucoes normais
#define MODO_DEF    1    // Modo de definicao: guardando linhas na tabela
#define MODO_EXP    2    // Modo de expansao: substituindo chamadas pelo corpo
#endif


#ifndef CONTEXTO_DEF_DEFINIDO
#define CONTEXTO_DEF_DEFINIDO
// Quando temos uma macro dentro de outra cada uma delas vai ganhar NivelDef tipo um nivel
typedef struct {
    char nome[MAX_NOME];                  
    char params[MAX_PARAMS][MAX_NOME];    // Lista dos parametros
    int  qtd_params;                      // Quantos parametros essa macro tem
} NivelDef;

// Estrutura principal da sua parte que basicamente vai controlar o modo de definicao.
typedef struct {
    int            d;                  // Contador de nivel de definicao
    NivelDef       pilha[MAX_NIVEL];   // Pilha que guarda os parametros de cada nivel
    DefinicaoMacro atual;              
} ContextoDef;
#endif



// Daqui ate a proxima divisoria eh so para mexer em texto

static int eh_alnum(char c) {  // Aqui ele basicamente vai verificar se um caractere pode fazer parte de um nome de parametro ou instrucao
    if (isalnum((unsigned char)c) || c == '_') {
        return 1;
    }
    return 0;
}



static void para_maiusculo(const char *orig, char *dest, int max) { // Essa aqui so muda as letras para maiusculo
    int i = 0;
    while (orig[i] != '\0' && i < max - 1) {
        dest[i] = (char)toupper((unsigned char)orig[i]);
        i++;
    }
    dest[i] = '\0';
}

static int separar_params(const char *lista_params, char params[][MAX_NOME], int max_params) {
    int qtd = 0;
    const char *p = lista_params;

    if (p == NULL) {
        return 0;
    }
    while (*p != '\0' && isspace((unsigned char)*p)) {
        p++;
    }
    if (*p == '\0') {
        return 0;
    }

    while (*p != '\0') {
        if (qtd >= max_params) {
            return -1;
        }
        while (*p != '\0' && isspace((unsigned char)*p)) {
            p++;
        }
        if (*p == '&') {
            p++;
        }

        char bruto[MAX_NOME];
        int tam = 0;
        while (*p != '\0' && *p != ',' && *p != '=' && !isspace((unsigned char)*p)) {
            if (!eh_alnum(*p)) {
                return -1;
            }
            if (tam < MAX_NOME - 1) {
                bruto[tam++] = *p;
            }
            p++;
        }
        bruto[tam] = '\0';
        if (tam == 0) {
            return -1;
        }
        para_maiusculo(bruto, params[qtd], MAX_NOME);
        qtd++;

        while (*p != '\0' && *p != ',') {
            p++;
        }
        if (*p == ',') {
            p++;
        }
    }
    return qtd;
}

static int eh_mcdefn(const char *linha, char *nome, char *params) {
    MacroLinhaInfo info;
    char erro[128] = "";

    if (!macro_analisar_linha(linha, &info, erro, sizeof(erro))) {
        return 0;
    }
    if (info.tipo == MACRO_LINHA_DEFINICAO_V1) {
        snprintf(nome, MAX_NOME, "%s", info.prototipo.nome);
        params[0] = '\0';
        for (size_t i = 0; i < info.prototipo.quantidadeParametros; i++) {
            if (i > 0) {
                strncat(params, ",", TAM_LINHA - strlen(params) - 1);
            }
            strncat(params, info.prototipo.parametros[i].nome, TAM_LINHA - strlen(params) - 1);
        }
        return 1;
    }
    if (info.tipo == MACRO_LINHA_DELIMITADOR_V2) {
        snprintf(nome, MAX_NOME, "MCDEFN");
        params[0] = '\0';
        return 1;
    }
    return 0;
}

static int eh_mcend(const char *linha) {
    MacroLinhaInfo info;
    char erro[128] = "";

    if (!macro_analisar_linha(linha, &info, erro, sizeof(erro))) {
        return 0;
    }
    return info.tipo == MACRO_LINHA_FIM_DEFINICAO;
}

static int salvar_macro(DefinicaoMacro *m) {
    return m != NULL && m->nome[0] != '\0';
}


// Esssa parte vai controlar a pilha de paremetros e niveis


void iniciar_contexto(ContextoDef *ctx) { // Ele vai limpa toda a estrutura e garantir que o contador de nivel comece em 0
    memset(ctx, 0, sizeof(ContextoDef));
    ctx->d = 0;
}


int empilhar_nivel( // Serve para subir o contador de nivel e guarda os parametros do macro no topo da pilha.
    ContextoDef *ctx,
    const char *nome,
    const char *lista_params,
    char *erro,
    size_t tam_erro
) {
    int novo_d = ctx->d + 1;

    
    if (novo_d >= MAX_NIVEL) { // Eh uma garantia pra nao passar do tamnaho maximo da pilha
        snprintf(erro, tam_erro, "Erro: limite de definicoes aninhadas excedido.");
        return 0;
    }

    // Pega a gaveta da pilha correspondente ao novo nivel
    NivelDef *nivel = &ctx->pilha[novo_d];
    memset(nivel, 0, sizeof(NivelDef));

    // Salva o nome da macro mudando para maiusculo nesse nivel
    para_maiusculo(nome, nivel->nome, MAX_NOME);

    int qtd = separar_params(lista_params, nivel->params, MAX_PARAMS);
    if (qtd < 0) {
        snprintf(erro, tam_erro, "Erro: parametros invalidos na macro %s.", nome);
        return 0;
    }

    nivel->qtd_params = qtd;
    ctx->d = novo_d; // Acontece a subida de nivel
    return 1;
}


// Chamada toda vez que um MCEND aparece.
void desempilhar_nivel(ContextoDef *ctx) { // Ela vai limpar o nivel atual da pilha e diminui o contador
    if (ctx->d > 0) {
        memset(&ctx->pilha[ctx->d], 0, sizeof(NivelDef));
        ctx->d--;
    }
}

// Procura se uma palavra da linha eh um parametro formal.
// Se encontrar ele devolve em qual nivel d_achado e em qual posicao i_achado ele esta.
static int buscar_param(
    const ContextoDef *ctx,
    const char *palavra,
    int *d_achado,
    int *i_achado
) {
    char busca[MAX_NOME];

    if (palavra[0] == '&') {
        palavra++;
    }

    para_maiusculo(palavra, busca, MAX_NOME);

    for (int nivel = ctx->d; nivel >= 1; nivel--) { // Aqui ele vai usar a regra de comecar pelo nivel mais interno e descer ate o nivel 1
        const NivelDef *item = &ctx->pilha[nivel]; // pq se acontecer de macros terem um parametro com mesmo nome o interno tem prioridade

        for (int i = 0; i < item->qtd_params; i++) {
            const char *param = item->params[i];
            if (param[0] == '&') {
                param++;
            }

            if (strcmp(busca, param) == 0) { 
                *d_achado = nivel;   
                *i_achado = i;   
                return 1;
            }
        }
    }

    return 0;
}

// Aqui ele vai substituir os parametros pela notacao (d,i)

// Ele percorre uma linha do corpo da macro e troca os parametros por #(d,i).
void trocar_params(const ContextoDef *ctx, const char *linha, char *saida) {
    int i = 0;     // Indice de leitura na linha original
    int j = 0;     // Indice de escrita na linha de saida
    int aspas = 0; 
    const char *inicio_trim = linha;

    while (*inicio_trim != '\0' && isspace((unsigned char)*inicio_trim)) {
        inicio_trim++;
    }
    if (*inicio_trim == '.' || *inicio_trim == ';') {
        snprintf(saida, TAM_LINHA, "%s", linha);
        return;
    }

    while (linha[i] != '\0' && j < TAM_LINHA - 1) {
        char c = linha[i];

        // Se encontrar aspas simples inverte a flag 'aspas'.
        if (c == '\'') {
            aspas = !aspas;
            saida[j++] = linha[i++];
            continue;
        }

        // Se encontrar ponto e virgula fora de aspas o resto da linha eh comentario.
        // Vai so copiar o comentario ate o fim sem fazer substituicao.
        if (!aspas && c == ';') {
            while (linha[i] != '\0' && j < TAM_LINHA - 1) {
                saida[j++] = linha[i++];
            }
            break;
        }

        if (!aspas && c == '#' && linha[i + 1] == '(' && isdigit((unsigned char)linha[i + 2])) {
            int d_ant = 0;
            int pos_ant = 0;
            int lidos = 0;

            if (sscanf(&linha[i], "#(%d,%d)%n", &d_ant, &pos_ant, &lidos) == 2 && lidos > 0) {
                char tag[32];
                snprintf(tag, sizeof(tag), "#(%d,%d)", d_ant, pos_ant);

                // Copia o novo marcador #(d,i) para a saida
                for (int k = 0; tag[k] != '\0' && j < TAM_LINHA - 1; k++) {
                    saida[j++] = tag[k];
                }

                i += lidos; // Pula os caracteres do #(d,i) original
                continue;
            }
        }

        // Se encontrou o comeco de uma palavra com _ ou & vai verificar se eh um paremetro
        if (!aspas && (isalpha((unsigned char)c) || c == '_' || c == '&')) {
            char palavra[MAX_NOME];
            int tam = 0;
            int inicio = i; // Guarda onde a palavra comecou

            if (c == '&') {
                palavra[tam++] = linha[i++];
            }

            // Continua lendo enquanto for letra, numero ou _
            while (eh_alnum(linha[i])) {
                if (tam < MAX_NOME - 1) {
                    palavra[tam++] = linha[i];
                }
                i++;
            }
            palavra[tam] = '\0';

            int d_param = 0;
            int i_param = 0;

            // Pergunta para a pilha se essa palavra e um parametro formal
            if (linha[i] != '=' && tam > 0 && buscar_param(ctx, palavra, &d_param, &i_param)) {
                // Se for monta o texto #(d,i) e escreve na saida no lugar da palavra
                char tag[32];
                snprintf(tag, sizeof(tag), "#(%d,%d)", d_param, i_param);

                for (int k = 0; tag[k] != '\0' && j < TAM_LINHA - 1; k++) {
                    saida[j++] = tag[k];
                }
            } else {
                // Se nao eh uma palavra comum copia exatamente como estava na linha original
                for (int p = inicio; p < i && j < TAM_LINHA - 1; p++) {
                    saida[j++] = linha[p];
                }
            }
            continue;
        }

        // Qualquer outro caractere eh copiado direto
        saida[j++] = linha[i++];
    }

    saida[j] = '\0';
}


// ============================================================================
// 5. FUNCOES PRINCIPAIS DO MODO DE DEFINICAO (CHAMADAS PELOS OUTROS MODULOS)
// ============================================================================

// Aqui quando inificar uma definicao ele va colocar no nivel d 1
int iniciar_def(
    ContextoDef *ctx,
    const char *nome,
    const char *params,
    char *erro,
    size_t tam_erro
) {
    iniciar_contexto(ctx);

    if (nome == NULL || nome[0] == '\0') {
        snprintf(erro, tam_erro, "Erro: macro sem nome.");
        return 0;
    }

    // Coloca a macro que estamos definindo no nivel d 1 da pilha
    if (!empilhar_nivel(ctx, nome, params, erro, tam_erro)) {
        return 0;
    }

    // Preenche o cabecalho da struct DefinicaoMacro
    DefinicaoMacro *m = &ctx->atual;
    strcpy(m->nome, ctx->pilha[1].nome);
    m->nparametros = (unsigned)ctx->pilha[1].qtd_params;
    m->nlinhas = 0;

    for (unsigned i = 0; i < m->nparametros; i++) {
        strcpy(m->parametros[i], ctx->pilha[1].params[i]);
    }

    return 1;
}

// Funcao que vai adiciona uma linha ao corpo da macro que esta sendo construida
static int salvar_linha(DefinicaoMacro *m, const char *linha, char *erro, size_t tam_erro) {
    if (m->nlinhas >= MAX_LINHAS) {
        snprintf(erro, tam_erro, "Erro: macro %s excedeu o limite de linhas.", m->nome);
        return 0;
    }

    if (!mc_adicionar_linha(NULL, m, linha)) {
        snprintf(erro, tam_erro, "Erro: memoria insuficiente na macro %s.", m->nome);
        return 0;
    }
    return 1;
}


// Devolve:
//   MODO_DEF (1)    = se ainda estamos dentro da definicao da macro
//   MODO_NORMAL (0) = se encontramos o MCEND externo (d chegou a 0) e terminamos
//   -1              = se deu algum erro
int processar_linha_def(
    ContextoDef *ctx,
    const char *linha,
    char *erro,
    size_t tam_erro
) {
    char nome_int[MAX_NOME];
    char params_int[TAM_LINHA];

    // CASO 1: A linha atual eh outro MCDEFN logo Temos uma definicao aninhada
    if (eh_mcdefn(linha, nome_int, params_int)) {
        // Aumenta o contador d e empilha os parametros da interna
        if (!empilhar_nivel(ctx, nome_int, params_int, erro, tam_erro)) {
            return -1;
        }
        // Guarda o cabecalho da macro interna dentro do corpo da externa e ela so sera registrada na tabela de macros quando a externa for expandida
        if (!salvar_linha(&ctx->atual, linha, erro, tam_erro)) {
            return -1;
        }
        return MODO_DEF;
    }

    // CASO 2: A linha atual eh um MCEND
    if (eh_mcend(linha)) {
        // Se d > 1, esse MCEND pertence a uma macro INTERNA
        // Entao nao podemos encerrar a macro externa entao apenas guardamos a linha e diminuimos d.
        if (ctx->d > 1) {
            if (!salvar_linha(&ctx->atual, linha, erro, tam_erro)) {
                return -1;
            }
            desempilhar_nivel(ctx); // d diminui
            return MODO_DEF;        
        }

        // Se d == 1, esse MCEND eh o da macro EXTERNA
        desempilhar_nivel(ctx); // d chega a 0

        // Salva a macro pronta na tabela de definicoes
        if (!salvar_macro(&ctx->atual)) {
            snprintf(erro, tam_erro, "Erro: falha ao salvar macro %s.", ctx->atual.nome);
            return -1;
        }

        return MODO_NORMAL; // Volta ao modo normal
    }

    // CASO 3: Eh uma instrucao normal dentro da macro
    // Substitui os parametros formais por #(d,i) e guarda no corpo da macro
    char convertida[TAM_LINHA];
    trocar_params(ctx, linha, convertida);

    if (!salvar_linha(&ctx->atual, convertida, erro, tam_erro)) {
        return -1;
    }

    return MODO_DEF;
}

// Quando encontra um MCDEFN lendo o arquivo
// Ela le todas as linhas ate o MCEND externo fechar que eh quando d volta a 0
int definir_macro(
    FILE *arq,
    const char *nome,
    const char *params,
    char *erro,
    size_t tam_erro
) {
    ContextoDef ctx;

    if (!iniciar_def(&ctx, nome, params, erro, tam_erro)) {
        return 0;
    }

    char linha[TAM_LINHA];
    while (fgets(linha, sizeof(linha), arq) != NULL) {
        // Remove o '\n' ou '\r' do final da linha lida
        size_t n = strlen(linha);
        while (n > 0 && (linha[n - 1] == '\n' || linha[n - 1] == '\r')) {
            linha[--n] = '\0';
        }

        int modo = processar_linha_def(&ctx, linha, erro, tam_erro);
        if (modo < 0) {
            return 0; // Ocorreu erro
        }
        if (modo == MODO_NORMAL) {
            return 1; // Conseguiu fechar o MCEND externo
        }
    }

    // Se o arquivo acabou antes do nivel d voltar para 0, faltou um MCEND
    snprintf(erro, tam_erro, "Erro: MCEND ausente na macro %s.", nome);
    return 0;
}

// Quando ele esta expandindo uma macro externa e encontra um interna MCDEFN
// la dentro ele passa as linhas para esta funcao registrar a macro interna na tabela.
int definir_macro_interna(
    const char **linhas,
    int total,
    int *lidas,
    char *erro,
    size_t tam_erro
) {
    if (linhas == NULL || total <= 0) {
        snprintf(erro, tam_erro, "Erro: definicao interna vazia.");
        return 0;
    }

    char nome[MAX_NOME];
    char params[TAM_LINHA];

    // Confere a primeira linha para extrair nome e parametros
    if (!eh_mcdefn(linhas[0], nome, params)) {
        snprintf(erro, tam_erro, "Erro: esperado MCDEFN na definicao interna.");
        return 0;
    }

    ContextoDef ctx;
    if (!iniciar_def(&ctx, nome, params, erro, tam_erro)) {
        return 0;
    }

    // Processa as linhas seguintes ate fechar o MCEND da macro interna
    for (int i = 1; i < total; i++) {
        int modo = processar_linha_def(&ctx, linhas[i], erro, tam_erro);
        if (modo < 0) {
            return 0;
        }
        if (modo == MODO_NORMAL) {
            //Informa quantas linhas da macro externa foram consumidas
            if (lidas != NULL) {
                *lidas = i + 1;
            }
            return 1;
        }
    }

    snprintf(erro, tam_erro, "Erro: MCEND ausente na macro interna %s.", nome);
    return 0;
}
