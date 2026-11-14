set -eu
cd "$(dirname "$0")/.."

CC="${CC:-gcc}"
CFLAGS="-std=c11 -Wall -Wextra -Wpedantic -O2 -I."
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

echo "== 1/3: Nucleo SIC/XE (teste de integracao) =="
"$CC" $CFLAGS teste_sim_real.c memoria.c decodificador.c Aritimetica.c \
  instrucoes_movimentacao_desvios.c sim_real.c -lm -o "$TMP_DIR/teste_sim_real"
"$TMP_DIR/teste_sim_real" exemplo.obj

echo
echo "== 2/3: Parser de macros =="
"$CC" $CFLAGS testes/teste_macros_parser.c montagem/macros_parser.c \
  -o "$TMP_DIR/teste_macros_parser"
"$TMP_DIR/teste_macros_parser"

echo
echo "== 3/3: Expansao de macros =="
"$CC" $CFLAGS testes/teste_expansao.c montagem/macros_expansao.c \
  -o "$TMP_DIR/teste_expansao"
"$TMP_DIR/teste_expansao"

echo
echo "TODOS OS GRUPOS DE TESTE EXECUTADOS COM SUCESSO."
