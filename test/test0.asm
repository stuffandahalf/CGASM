	.org 0x100

_start:
    abx
    adca #$5

.str: db "Hello World", 0x0a, 0x0d, 0
