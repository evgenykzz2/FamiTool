FOUR_PLAYERS=0
MAPPER=4

	.inesprg 2
	.ineschr 2
	.inesmir 0
	.inesmap MAPPER

	.include "regs.inc"
	.include "mem.inc"
	
	.bank 0
	.org $8000
	
	.bank 1
	.org $A000
	
	.bank 2
	.org $C000
	
	.bank 3
	.org $E000
	.include "main.inc"
	.include "api.inc"
	.include "screen_jax.inc"

	.org $FFFA  ; interrupt vector
	.dw main_nmi     ; NMI address
	.dw main_reset   ; Start address
	.dw main_irq     ; IRQ address

	.bank 4
	.incbin "jax_chr.chr"