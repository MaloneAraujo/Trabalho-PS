# Joao Victor - Parte 4: expansao e chamadas aninhadas

Este pacote e a copia isolada do modulo usado na implementacao completa de
reserva da parte 3. Nao e um segundo processador de macros e nao substitui
o trabalho dos outros integrantes. Esta entrega contem somente o modulo de
expansao, os contratos compartilhados propostos e o teste isolado.

## O que e sua parte

- `montagem/macros_expansao.c`: implementacao do mecanismo de expansao.
- `testes/teste_expansao.c`: teste isolado com dubles dos outros modulos.
- `montagem/macros_interno.h` e `macros.h`: contratos compartilhados para
  compilar. Sao a proposta de estruturas da implementacao completa, nao uma
  exigencia de que os colegas descartem estruturas que ja desenvolveram.

Nao copie os dubles do teste para o programa final. Eles implementam somente
o necessario para o teste e nao sao um parser ou uma tabela de macros reais.

## Compilar e testar sua parte

No PowerShell, nesta pasta, com GCC no PATH:

```powershell
.\testar-parte4.ps1
```

Ou diretamente:

```text
gcc -std=c11 -Wall -Wextra -Wpedantic -I. testes/teste_expansao.c montagem/macros_expansao.c -o teste_expansao.exe
```

Depois execute `./teste_expansao.exe`. Resultado verificado em 13/11/2026:
**27 verificacoes passaram**, incluindo duas chamadas de SOMA dentro de DUPLA,
retomada da macro anterior, substituicao de argumentos, serial por chamada,
adiamento de .SER em definicoes internas e rejeicao de recursao/limites.

## Funcoes para explicar

### mc_empilhar

Recebe a definicao da macro e seus argumentos reais. Confere a quantidade,
o limite da pilha e se o mesmo nome ja esta em expansao (recursao direta ou
indireta). Cria um quadro com a macro, copia dos argumentos, proxima linha
e numero de expansao exclusivo. Nao executa nenhuma instrucao SIC/XE.

### mc_proxima_expansao

Entrega uma linha do quadro no topo. Incrementa o cursor antes de entregar
a linha. Se essa linha chamar outra macro, o controlador pode empilhar a
chamada; quando ela terminar, a proxima consulta retoma o cursor anterior.
Remove quadros terminados. Retornos: 1=linha, 0=pilha vazia, -1=erro.

### substituir (funcao interna)

Transforma os marcadores de parametros `#(d,i)` em argumentos reais. O nivel
1 pertence a macro atual; niveis superiores pertencem a definicoes internas
e sao reduzidos em um, sem substituicao prematura. Nao modifica strings ou
comentarios e nao volta a processar o texto inserido como argumento.
Substitui `.SER` pelo numero da chamada atual, exceto dentro de definicoes
internas, onde essa substituicao precisa esperar a expansao interna.

### anexar (funcao interna)

Monta a linha resultante com verificacao de tamanho, evitando ultrapassar
o buffer. Linha excessiva gera erro, nunca truncamento silencioso.

## Como integrar com os colegas

1. Com Malone, alinhe `McMacro`, `McQuadro` e `McContexto` do cabecalho.
   Definicoes antigas devem continuar vivas enquanto houver quadros que as
   referenciem; nesta versao todas ficam encadeadas ate o final do processamento.
2. Com Gabriel Silveira, alinhe `mc_parse`, `mc_igual` e a separacao de argumentos.
   As funcoes esperadas estao declaradas em `macros_interno.h`.
3. Com Gabriel Britto, alinhe a codificacao de parametros `#(d,i)`, com nivel
   iniciado em 1 e indice em 0. O controlador coleta definicoes; este modulo
   apenas entrega suas linhas e acompanha a profundidade para adiar `.SER`.
4. O controlador consulta a pilha antes de ler outra linha do arquivo original.
   Ao reconhecer uma chamada, busca a macro e chama `mc_empilhar`. Ele deve
   reprocessar a linha retornada, nao escreve-la diretamente sem verificar se
   e outra chamada ou uma definicao interna.
5. Ao receber 0, o controlador pode voltar ao arquivo original. Ao receber -1,
   interrompe o processamento e apresenta a mensagem de erro.
6. O integrador dos colegas gera MASMAPRG.ASM e chama o montador. Este pacote
   isolado nao faz isso: sua responsabilidade termina na entrega do texto.

Fluxo central do controlador (pseudocodigo):

```text
linha = proxima expansao, ou proxima linha do arquivo se a pilha estiver vazia
se for definicao: coletar e cadastrar
se for chamada: buscar a macro, separar argumentos e empilhar
senao: escrever a linha na saida
```

## Limites e observacoes

16 parametros, 32 quadros de expansao, nomes de ate 31 caracteres e linhas
de ate 511 caracteres mais terminador; ha limites de 100000 expansoes/passos.
Argumentos sao posicionais. `.SER` e maiusculo; use prefixo como `L.SER`
para formar um rotulo SIC/XE valido. Recursao de nome ativo e proibida.
Parametros devem ser vetores terminados em NUL conforme o contrato C.

O teste isolado nao prova a integracao completa. Os testes do outro pacote
validam parser, definicoes aninhadas, expansao, montagem e simulador juntos.
Compare os contratos com o que os colegas ja fizeram antes de copiar arquivos.
