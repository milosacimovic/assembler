.text
.global START
START: LOAD R0, #3
PUSH R0
CALL sub
POP R13
LOAD R0, #0
INT R0
.end
