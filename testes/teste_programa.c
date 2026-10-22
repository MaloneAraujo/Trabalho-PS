#include "programa.h"
#include <stdio.h>
#include <string.h>

static unsigned passou, falhou;
static void check(int ok, const char *nome) {
    if (ok) ++passou;
    else { ++falhou; printf("FALHOU: %s\n",nome); }
}
int main(void) {
    Maquina *m = sim_criar();
    char erro[320];
    uint8_t imagem[MEM_TAMANHO], anterior[MEM_TAMANHO];
    uint8_t pequeno[] = {0x01,0x00,0x05,0x3F,0x00,0x03};
    unsigned i;
    int r = SIM_OK;
    if (!m) return 1;
    check(!programa_carregar(m,"exemplos/soma.asm",erro,sizeof erro),"monta .asm na memoria");
    memcpy(imagem,m->mem,sizeof imagem);
    check(!programa_carregar(m,"exemplos/soma.obj",erro,sizeof erro),"carrega .obj");
    check(!memcmp(imagem,m->mem,sizeof imagem),"imagem ASM igual ao objeto CLI");
    for (i=0; i<30 && r==SIM_OK; ++i) r=sim_passo(m,erro,sizeof erro);
    check(r==SIM_FIM && m->A==8 && m->B==7 && m->mem[32]==8,"executa programa da interface");
    memcpy(anterior,m->mem,sizeof anterior);
    check(programa_carregar(m,"testes/entrada_invalida.asm",erro,sizeof erro)==-1 && strstr(erro,"indefinido"),"erro do montador legivel");
    check(m->A==8 && m->PC==21 && !memcmp(anterior,m->mem,sizeof anterior),"falha de montagem preserva estado");
    check(programa_carregar(m,"nao-existe.asm",erro,sizeof erro)==-1,"arquivo ausente rejeitado");
    check(m->A==8 && !memcmp(anterior,m->mem,sizeof anterior),"arquivo ausente preserva estado");
    check(!sim_carregar_imagem(m,imagem,sizeof imagem,0,erro,sizeof erro),"reiniciar pela imagem original");
    check(m->A==0 && m->B==0 && m->PC==0 && m->mem[32]==0,"reinicio restaura registradores e dados");
    check(!programa_carregar(m,"exemplos/enderecamento.asm",erro,sizeof erro) && m->PC==0x100,"entrada nao zero");
    check(sim_carregar_imagem(m,imagem,sizeof imagem+1,0,erro,sizeof erro)==-1,"imagem grande rejeitada");
    check(m->PC==0x100,"imagem invalida preserva PC");
    check(sim_carregar_imagem(m,imagem,0,0,erro,sizeof erro)==-1,"imagem vazia rejeitada");
    check(sim_carregar_imagem(m,imagem,sizeof imagem,MEM_TAMANHO,erro,sizeof erro)==-1,"entrada invalida rejeitada");
    check(sim_carregar_imagem(m,NULL,sizeof imagem,0,erro,sizeof erro)==-1,"origem nula rejeitada");
    memcpy(anterior,m->mem,sizeof anterior);
    check(!sim_carregar_imagem(m,m->mem,m->mem_tam,0x100,erro,sizeof erro) &&
          !memcmp(anterior,m->mem,sizeof anterior),"origem pode ser a propria memoria");
    check(!sim_carregar_imagem(m,pequeno,sizeof pequeno,0,erro,sizeof erro) && m->mem[0x100]==0,"imagem curta zera restante");
    check(sim_passo(m,erro,sizeof erro)==SIM_OK && m->A==5,"imagem curta executa");
    sim_destruir(m);
    printf("Ponte grafica: %u passaram; %u falharam.\n",passou,falhou);
    return falhou ? 1 : 0;
}
