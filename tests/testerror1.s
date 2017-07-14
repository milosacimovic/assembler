.text
ADD R0, R0, R1, R2 ; dodatni operandi
label:
OR R0, R1, R2
JZ R0
3hello: ;neispravna labela
ASL R0, R1, R2 ASR R1, R2, R3 ;vise instrukcija u jednom redu
label:
SUB R1, R1, R1  ;visestruka definicija labele
l1: l2: ; vise labela u jednoj liniji

ADD R17, R2, R3 ; nepostojeci registar
STORE R1, #0x123 ;ne sme neposredno
LOAD R1, 0x8000080800080 ; preveliki broj
