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
in_reg DEF 0xA4
.global START
START:
LOAD R0, #0 ; ucitava 0 u registar R0
x: JZ R0,x ; apsolutni skok na lokaciju x
.text.1
fault:
LOADUB R2, #'g'
STOREB R2, out_reg
LOADUB R2, #'r'
STOREB R2, out_reg
LOADUB R2, #'e'
STOREB R2, out_reg
LOADUB R2, #'s'
STOREB R2, out_reg
LOADUB R2, #'k'
STOREB R2, out_reg
LOADUB R2, #'a'
STOREB R2, out_reg
LOADUB R2, #'\n'
STOREB R2, out_reg
INT R0
.text.2
keypress:
PUSH R2
LOADUB R2, #'p'
STOREB R2, out_reg
LOADUB R2, #'r'
STOREB R2, out_reg
LOADUB R2, #'i'
STOREB R2, out_reg
LOADUB R2, #'t'
STOREB R2, out_reg
LOADUB R2, #'i'
STOREB R2, out_reg
LOADUB R2, #'s'
STOREB R2, out_reg
LOADUB R2, #'n'
STOREB R2, out_reg
LOADUB R2, #'u'
STOREB R2, out_reg
LOADUB R2, #'t'
STOREB R2, out_reg
LOADUB R2, #' '
STOREB R2, out_reg
LOADUB R2, in_reg
STOREB R2, out_reg
LOADUB R2, #'\n'
STOREB R2, out_reg
POP R2
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
RET
.bss
sp:
DB 0x1000 DUP ?
.end
