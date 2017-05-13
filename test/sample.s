.data
a: .long a
ORG 0x20
.text
LOAD R1, a ; ucitava sadrzaj memorijske lokacije a u registar R1
LOAD R2, #a ; ucitava adresu lokacije a u registar R2
LOAD R3, $a ; ucitava sadrzaj lokacije a u registar R3 koristeci PC relativno adresiranje
x: JZ R0, x ; apsolutni skok na lokaciju x
JZ R0, $x ; PC relativni skok na lokaciju x
.end