# Relatório de testes — Processador de macros SIC/XE

**Data:** 13/11/2026  
**Escopo:** testes dos módulos de parser, tabela de macros e expansão presentes no pacote recebido.

## Resultado geral

| Módulo | Verificações | Aprovadas | Falhas |
|---|---:|---:|---:|
| Parser (`macros_parser.c`) | 45 | 45 | 0 |
| Expansão (`macros_expansao.c`) | 27 | 27 | 0 |
| Tabela (`macros_tabela.c`) | 13 | 13 | 0 |
| **Total** | **85** | **85** | **0** |

Os três conjuntos foram compilados com GCC usando `-std=c11 -Wall -Wextra -Wpedantic -Werror`. Também foram executados com AddressSanitizer e UndefinedBehaviorSanitizer (`-fsanitize=address,undefined`), com detecção de vazamentos habilitada. Não foram observados erros de memória, comportamento indefinido ou vazamentos nos cenários executados.

## Cobertura

### Parser — 45 verificações

- Reconhecimento das sintaxes de macro V1 e V2 e do delimitador `MCEND`.
- Separação de protótipos, rótulos, operações, operandos e comentários.
- Argumentos posicionais e por palavra-chave, parâmetros opcionais e valores padrão.
- Tratamento de aspas, vírgulas em constantes, identificadores e linhas vazias.
- Rejeição de parâmetros duplicados, argumentos inválidos, campos excedentes, aspas não fechadas e limites excedidos.
- Tratamento de ponteiros nulos e entradas inválidas.

### Expansão — 27 verificações

- Empilhamento e consumo de linhas de expansão.
- Substituição de parâmetros e preservação de argumentos inseridos.
- Chamadas aninhadas e retomada da macro anterior.
- Numeração serial por chamada e adiamento da substituição de `.SER` em definições internas.
- Rejeição de recursão, quantidade incorreta de argumentos e limites de expansão/linha.

### Tabela — 13 verificações novas

- Criação e liberação do contexto.
- Busca sem diferenciar maiúsculas de minúsculas.
- Cadastro, armazenamento ordenado do corpo e inclusão de parâmetros.
- Tratamento de macro inexistente e ponteiros nulos.
- Redefinição mantendo a versão anterior encadeada.
- Rejeição de quantidade de parâmetros acima do limite.

O teste novo está em `testes/teste_macros_tabela.c`.

## Como reproduzir

Na raiz do projeto:

```sh
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -Imontagem montagem/macros_parser.c testes/teste_macros_parser.c -o teste_macros_parser
./teste_macros_parser

gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -I. testes/teste_expansao.c montagem/macros_expansao.c -o teste_expansao
./teste_expansao

gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -Imontagem montagem/macros_tabela.c testes/teste_macros_tabela.c -o teste_macros_tabela
./teste_macros_tabela
```

Para repetir a verificação com sanitizadores, acrescente à compilação:

```sh
-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer
```

E execute com `ASAN_OPTIONS=detect_leaks=1`.

## Limitação importante: integração ponta a ponta

O pacote recebido contém os módulos e testes isolados de parser, tabela e expansão, mas não contém a implementação do controlador `processar_macros` declarada em `montagem/macros.h`. A rotina `montar_com_macros` depende dessa implementação e também do montador completo (`montador.h` e módulos correspondentes), ausentes nesta cópia. Portanto, não foi possível afirmar que a leitura integral de um fonte, a coleta de definições, a expansão integrada e a geração de `MASMAPRG.ASM`/objeto funcionam em conjunto.

**Conclusão:** os 85 testes unitários executados passaram. A validação isolada dos três módulos foi concluída; a integração ponta a ponta permanece pendente da disponibilização do controlador e do montador completos.
