$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    & gcc -std=c11 -Wall -Wextra -Wpedantic -I. testes/teste_expansao.c montagem/macros_expansao.c -o teste_expansao.exe
    if ($LASTEXITCODE -ne 0) { throw 'Falha na compilacao' }
    & .\teste_expansao.exe
    if ($LASTEXITCODE -ne 0) { throw 'Falha nos testes' }
} finally { Pop-Location }
