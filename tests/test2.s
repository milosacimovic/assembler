ORG 0x0
.data
DD sp ; stek inicijalna vrednost
DD 2 DUP ? ; prvi i drugi ulaz su trenutno prazni
DD fault ; sadrzi adresu funkcije handler greske
DD timer ; sadrzi adresu timer funkcije
DD keypress ; sadrzi adresu handler pritisnutog tastera
DD 36 DUP ?
.text.0
out_reg DEF 0xA0
.global START
START:
LOADUB R0, #'1'
LOAD R4, #10
PUSH R4
PUSH R0
CALL sub ;rezultat je u R0
POP R10
POP R10
l: JZ R3, $l
.text.1
fault:
LOAD R0, #0
INT R0
.text.2
keypress:
RET
.text.3
timer:
PUSH R2
LOADUB R2, #'t'
STOREB R2, out_reg
LOADUB R2, #'i'
STOREB R2, out_reg
LOADUB R2, #'m'
STOREB R2, out_reg
LOADUB R2, #'e'
STOREB R2, out_reg
LOADUB R2, #'r'
STOREB R2, out_reg
LOADUB R2, #'\n'
STOREB R2, out_reg
POP R2
RET
.text.4 ; potprogram za neko izracunavanje
sub:
PUSH R13
LOAD R13, SP
LOAD R3, #1
LOAD R0, [R13 -11] ; R0 = '1'
LOAD R1, [R13 - 15] ; R1 = '10'
loop:
STOREB R0, out_reg
ADD R0, R0, R3
SUB R1, R1, R3
JNZ R1, loop
POP R13
RET
.bss
sp:
DB 0x1000 DUP ?
.end
