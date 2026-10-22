. Demonstra formato 2, 4, endereco indireto e indexacao.
MODOS   START 100
INICIO  CLEAR X
        +LDA #65535
        STA RESULT
        LDA @PONTE
        LDX #1
        LDCH TEXTO,X
        RMO A,S
        SHIFTL S,16
FIM     J FIM
PONTE   WORD RESULT
RESULT  RESW 1
TEXTO   BYTE C'AB'
        END INICIO
