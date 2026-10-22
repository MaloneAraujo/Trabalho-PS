. Monta e executa uma soma, com chamada de subrotina e desvio condicional.
SOMA    START 0
INICIO  LDA #5
        ADD #3
        STA RESULT
        COMP #8
        JEQ CERTO
        LDA #99
CERTO   JSUB ROTINA
FIM     J FIM
ROTINA  LDB #7
        RSUB
RESULT  RESW 1
        END INICIO
