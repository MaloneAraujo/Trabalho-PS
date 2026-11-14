# Plano de testes — Simulador SIC/XE e macros

## 1. Objetivo

Definir os testes reproduzíveis para verificar o núcleo do simulador SIC/XE e os módulos de parser e expansão de macros presentes neste repositório. O plano limita-se aos fontes e contratos efetivamente encontrados no pacote.

## 2. Escopo

**Incluído**
- Carregamento e execução do programa de exemplo pelo núcleo SIC/XE.
- Atualização de registradores e memória durante instruções.
- Comparação, desvio condicional, chamada e retorno de subrotina.
- Detecção de término e reinicialização do simulador.
- Reconhecimento e validação de definições/chamadas de macros.
- Expansão de macros, substituição de parâmetros, chamadas aninhadas e proteção contra recursão/limites.

**Não coberto por esta execução**
- Interface gráfica Raylib e interação por mouse/teclado.
- Montagem completa de programas com integração de todos os módulos.
- Testes de desempenho, carga prolongada, segurança ou compatibilidade entre sistemas operacionais.
- Arquivos de teste antigos sem alvo correspondente ou dependências presentes no pacote.

## 3. Ambiente de referência

- Sistema usado na execução registrada: Linux x86-64.
- Compilador: GCC 14.2.0.
- Linguagem: C11.
- Opções: `-Wall -Wextra -Wpedantic -O2`.
- Dependências adicionais: biblioteca matemática (`-lm`) para o núcleo; nenhuma biblioteca externa para os testes de macros.

## 4. Casos de teste

| ID | Módulo | Cenário | Resultado esperado |
|---|---|---|---|
| NUC-01 | Núcleo | Carregar `exemplo.obj` | Carga concluída e PC no endereço inicial |
| NUC-02 | Núcleo | LDA #5 e ADD #3 | Registrador A termina com 8 |
| NUC-03 | Núcleo | STA 40 | Palavra armazenada conforme o exemplo |
| NUC-04 | Núcleo | COMP #8 e JEQ | Condição igual e desvio para 0x12 |
| NUC-05 | Núcleo | JSUB, LDB e RSUB | Retorno salvo em L e PC restaurado |
| NUC-06 | Núcleo | J para o próprio endereço | Retorno `SIM_FIM` |
| NUC-07 | Núcleo | `sim_reset` | Registradores zerados e PC reiniciado |
| MAC-P-01 | Parser | Definições V1/V2, MCEND e comentários | Tipos e campos reconhecidos corretamente |
| MAC-P-02 | Parser | Parâmetros posicionais e nomeados | Argumentos associados corretamente |
| MAC-P-03 | Parser | Sintaxe inválida, duplicidades e limites | Entrada rejeitada com erro |
| MAC-P-04 | Parser | Linhas com/sem rótulo e callback de operação | Componentes da linha separados corretamente |
| MAC-E-01 | Expansão | Substituição de parâmetros | Texto expandido com argumentos reais |
| MAC-E-02 | Expansão | Chamadas aninhadas | Retomada correta da macro chamadora |
| MAC-E-03 | Expansão | `.SER` e definições internas | Numeração aplicada no nível correto |
| MAC-E-04 | Expansão | Recursão e limites | Operação recusada com erro, sem expansão inválida |

## 5. Critérios de aprovação

- Compilação sem erros.
- Nenhuma asserção/verificação falha.
- Processo de cada grupo retorna código zero.
- Resultados observáveis correspondem aos resultados esperados.

## 6. Como executar

Na raiz do projeto, com GCC instalado:

```bash
bash testes/executar_testes.sh
```

É possível selecionar outro compilador C definindo `CC`, por exemplo:

```bash
CC=clang bash testes/executar_testes.sh
```

O script compila os executáveis em diretório temporário e remove esses arquivos ao terminar. Não modifica os fontes nem os executáveis distribuídos.
