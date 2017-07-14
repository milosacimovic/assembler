; overrlapping sections
ORG 0x0
.data
DD 300 DUP ?
ORG 0x30
.text
.global START
START: LOAD R0, #0
INT R0
