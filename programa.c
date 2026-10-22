#include "programa.h"
#include "montagem/montador.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

int programa_carregar(Maquina *m, const char *caminho, char *erro, size_t tamanho) {
    const char *extensao;
    FILE *fonte;
    Montagem *montagem;
    int ok, resultado;
    if (!m || !caminho) { snprintf(erro,tamanho,"maquina ou caminho invalido"); return -1; }
    extensao = strrchr(caminho, '.');
    if (!extensao || strlen(extensao) != 4 ||
        tolower((unsigned char)extensao[1]) != 'a' ||
        tolower((unsigned char)extensao[2]) != 's' ||
        tolower((unsigned char)extensao[3]) != 'm')
        return sim_carregar(m, caminho, erro, tamanho);
    fonte = fopen(caminho, "r");
    if (!fonte) { snprintf(erro,tamanho,"nao foi possivel abrir o assembly"); return -1; }
    montagem = calloc(1, sizeof *montagem);
    if (!montagem) { fclose(fonte); snprintf(erro,tamanho,"memoria insuficiente"); return -1; }
    ok = montar(fonte, montagem);
    fclose(fonte);
    if (!ok) {
        snprintf(erro,tamanho,"montador: %s",montagem->erro);
        free(montagem);
        return -1;
    }
    resultado = sim_carregar_imagem(m, montagem->imagem, sizeof montagem->imagem,
                                    montagem->entrada, erro, tamanho);
    free(montagem);
    return resultado;
}
