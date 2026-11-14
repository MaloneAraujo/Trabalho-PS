/* Parte 6: exercita o mesmo carregamento usado pela janela, sem abrir Raylib. */
#include "programa.h"
#include "montagem/montador.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned total, falhas;
#define CHECK(x, nome) do { ++total; if (!(x)) { ++falhas; printf("FALHA: %s (linha %d)\n",nome,__LINE__); } } while (0)
static int ler_expandido(char *texto, size_t capacidade) {
    FILE *f = fopen("MASMAPRG.ASM","r");
    size_t n;
    if (!f) return 0;
    n = fread(texto,1,capacidade-1,f); texto[n] = 0;
    fclose(f); return 1;
}
int main(void) {
    Maquina *m = sim_criar();
    Montagem *montagem = calloc(1,sizeof *montagem);
    char erro[320], texto[16000], anterior[16000];
    uint8_t inicial[MEM_TAMANHO], memoria_anterior[MEM_TAMANHO];
    uint32_t pc_anterior, a_anterior;
    unsigned i, endereco_resultado = MEM_TAMANHO;
    FILE *f;
    int r = SIM_OK, ok;
    CHECK(m && montagem,"alocacao");
    if (!m || !montagem) { free(montagem); if (m) sim_destruir(m); return 1; }
    ok = programa_carregar(m,"exemplos/macros_aninhadas.asm",erro,sizeof erro);
    CHECK(ok==0,"interface carrega ASM com macros do grupo");
    if (ok) { printf("Detalhe: %s\n",erro); goto fim; }
    CHECK(ler_expandido(texto,sizeof texto),"gera MASMAPRG.ASM");
    /* Comentarios podem mencionar macros; montar verifica as linhas de codigo. */
    f=fopen("MASMAPRG.ASM","r");
    CHECK(f!=NULL,"abre fonte expandido");
    if (!f) goto fim;
    ok=montar(f,montagem); fclose(f);
    CHECK(ok,"fonte expandido aceito pelo montador comum");
    CHECK(ok && !memcmp(m->mem,montagem->imagem,MEM_TAMANHO),"imagem da GUI corresponde ao expandido");
    for (i=0;i<montagem->nsimbolos;++i)
        if (!strcmp(montagem->simbolos[i].nome,"RESULT")) endereco_resultado=(unsigned)montagem->simbolos[i].valor;
    CHECK(endereco_resultado < MEM_TAMANHO-2,"encontra RESULT");
    memcpy(inicial,m->mem,sizeof inicial);
    CHECK(sim_passo(m,erro,sizeof erro)==SIM_OK && m->A==5,"primeiro passo: LDA VALOR");
    CHECK(sim_passo(m,erro,sizeof erro)==SIM_OK && m->A==10,"segundo passo: ADD VALOR");
    CHECK(!sim_carregar_imagem(m,inicial,sizeof inicial,montagem->entrada,erro,sizeof erro),"reinicia imagem original");
    CHECK(m->A==0 && m->PC==montagem->entrada && !memcmp(m->mem,inicial,sizeof inicial),"reinicio restaura estado");
    for (i=0;i<100 && r==SIM_OK;++i) r=sim_passo(m,erro,sizeof erro);
    CHECK(r==SIM_FIM && m->A==10,"executa imagem com macros ate o fim");
    CHECK(endereco_resultado<MEM_TAMANHO-2 && m->mem[endereco_resultado]==0 &&
          m->mem[endereco_resultado+1]==0 && m->mem[endereco_resultado+2]==10,"RESULT recebe 10");
    memcpy(memoria_anterior,m->mem,sizeof memoria_anterior);
    pc_anterior=m->PC; a_anterior=m->A;
    CHECK(ler_expandido(anterior,sizeof anterior),"guarda fonte expandido");
    CHECK(programa_carregar(m,"testes/interface_macro_invalida.asm",erro,sizeof erro)==-1,"erro de macros chega a interface");
    CHECK(erro[0]!=0,"erro legivel");
    CHECK(m->PC==pc_anterior && m->A==a_anterior && !memcmp(m->mem,memoria_anterior,sizeof memoria_anterior),"erro de macro preserva maquina");
    CHECK(ler_expandido(texto,sizeof texto) && !strcmp(anterior,texto),"erro de macro preserva MASMAPRG.ASM");
    CHECK(programa_carregar(m,"arquivo-inexistente-parte6.asm",erro,sizeof erro)==-1,"arquivo ausente rejeitado");
    CHECK(m->PC==pc_anterior && m->A==a_anterior && !memcmp(m->mem,memoria_anterior,sizeof memoria_anterior),"arquivo ausente preserva maquina");
    CHECK(programa_carregar(m,"exemplos/macros_aninhadas.asm",erro,sizeof erro)==0 && m->A==0,"recarregar monta novamente");
    CHECK(programa_carregar(m,"exemplos/soma.obj",erro,sizeof erro)==0,"OBJ continua carregando diretamente");
    r=SIM_OK; for (i=0;i<100 && r==SIM_OK;++i) r=sim_passo(m,erro,sizeof erro);
    CHECK(r==SIM_FIM && m->A==8 && m->B==7,"OBJ continua executando");
    CHECK(ler_expandido(texto,sizeof texto) && !strcmp(anterior,texto),"carregar OBJ nao reescreve MASMAPRG.ASM");
fim:
    sim_destruir(m); free(montagem);
    printf("Interface/macros: %u verificacoes; %u falhas.\n",total,falhas);
    return falhas ? 1 : 0;
}
