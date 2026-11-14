# Relatório de testes — Simulador SIC/XE e macros

**Data da execução:** 13/11/2026  
**Resultado geral:** aprovado para os grupos executados neste ambiente.

## 1. Ambiente

- Linux x86-64
- GCC 14.2.0
- C11 com `-Wall -Wextra -Wpedantic -O2`

## 2. Resultados

| Grupo | Verificações aprovadas | Falhas | Resultado |
|---|---:|---:|---|
| Integração do núcleo SIC/XE (`teste_sim_real.c`) | 12 | 0 | APROVADO |
| Parser de macros (`testes/teste_macros_parser.c`) | 45 | 0 | APROVADO |
| Expansão de macros (`testes/teste_expansao.c`) | 27 | 0 | APROVADO |
| **Total** | **84** | **0** | **APROVADO** |

### 2.1 Núcleo SIC/XE

O programa `exemplo.obj` foi carregado e executado. O teste verificou carga e PC inicial, LDA, ADD, STA, COMP, JEQ, JSUB, LDB, RSUB, término por salto para si mesmo e `sim_reset`. Todas as verificações passaram.

### 2.2 Parser de macros

Foram aprovadas 45 verificações envolvendo definições V1 e V2, delimitadores, comentários, parâmetros posicionais e nomeados, chamadas, rótulos, separação de campos, entradas inválidas e limites de tamanho/quantidade.

### 2.3 Expansão de macros

Foram aprovadas 27 verificações envolvendo substituição de parâmetros, chamadas aninhadas, retomada da macro anterior, numeração `.SER`, definições internas, recursão e limites de expansão.

## 3. Limitações e ressalvas

- A execução foi feita em Linux; não representa validação de Windows.
- A interface gráfica não foi executada nem submetida a testes automatizados nesta rodada.
- O teste do núcleo é uma integração baseada no programa de exemplo, não uma cobertura exaustiva de todas as instruções SIC/XE.
- Os testes de parser e expansão exercitam módulos isolados; não comprovam, por si só, a integração completa do montador.
- O `Makefile` presente no pacote referencia alguns fontes e alvos que não estão presentes na árvore recebida. Por isso, a execução foi feita por comandos diretos, reunidos em `testes/executar_testes.sh`, sem alterar o Makefile ou o código de implementação.

## 4. Conclusão

Os três grupos executáveis selecionados compilaram e terminaram com sucesso: 84 verificações aprovadas e nenhuma falha. A aprovação se restringe aos cenários descritos neste relatório; a interface gráfica, a integração completa do montador e os demais cenários não foram certificados.
