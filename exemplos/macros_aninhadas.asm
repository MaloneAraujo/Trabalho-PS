. Exemplo de macros aninhadas para demonstracao do processador SIC/XE.
. Demonstra definicao de macro dentro de macro (macros aninhadas)
. e chamada de macro dentro de macro (chamadas aninhadas) em uma passagem.

        START 0000

. Macro externa que define internamente SOMA e DUPLA e invoca DUPLA.
        MCDEFN
        EXTERNA X=
        MCDEFN
        SOMA A=,B=
        LDA A
        ADD B
        STA RESULT
        MCEND
        MCDEFN
        DUPLA Y=
        SOMA A=Y,B=Y
        MCEND
        DUPLA Y=X
        MCEND

. Programa principal: a chamada EXTERNA registra SOMA e DUPLA e executa a chamada aninhada.
        EXTERNA X=VALOR
        J FIM

VALOR   WORD 5
RESULT  RESW 1
FIM     J FIM
        END
