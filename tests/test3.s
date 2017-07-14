ORG 0x0
.data
DD 42 DUP ?
.text
out_reg DEF 0xA0
.global START ; program za testiranje TNS-a
A DEF B
B DEF C
C DEF D
D DEF 'A'
START:
LOAD R0, #A
LOAD R3, #26
LOAD R4, #1
loop:
STOREB R0, out_reg
ADD R0, R0, R4
SUB R3, R3, R4
JNZ R3, $loop
INT R3
.end
