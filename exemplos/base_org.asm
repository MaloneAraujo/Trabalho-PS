. BASE informa o montador; LDB carrega de fato o registrador B.
BASES   START 0
INICIO  +LDB #DADOS
        BASE DADOS
        LDA DADOS
        NOBASE
FIM     J FIM
        ORG 3000
DADOS   WORD 42
TAMANHO EQU *-DADOS
        END INICIO
