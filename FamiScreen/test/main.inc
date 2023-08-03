;--------------------------------------------------------------------

JOY_A      = $80
JOY_B      = $40
JOY_SELECT = $20
JOY_START  = $10
JOY_UP     = $08
JOY_DOWN   = $04
JOY_LEFT   = $02
JOY_RIGHT  = $01

GAME_PPU_CTRL = $A8
GAME_PPU_MASK = $1E	;$1e - enable left column $18 - disable


;--------------------------------------------------------------------
main_reset:
    sei
    cld
    ldx #$40
    stx CTRL_PORT2
    ldx #$ff
    txs					;reset stack pointer
    inx
    stx PPU_MASK		
    stx DMC_FREQ
    stx PPU_CTRL		;no NMI
	
	jsr api_waitblank
	jsr api_ram_clear
	
    sta $E000   ; IRQ disable

	;0
	ldx #$00
	stx $8000
	ldy #$00
	sty $8001
	;1
	inx
	stx $8000
	iny
	iny
	sty $8001
	;2
	inx
	stx $8000
	iny
	iny
	sty $8001
	;3
	inx
	stx $8000
	iny
	sty $8001
	;4
	inx
	stx $8000
	iny
	sty $8001
	;5
	inx
	stx $8000
	iny
	sty $8001
	
	;6
	inx
	stx $8000
	ldy #$FC
	sty $8001
	
	;7
	inx
	stx $8000
	iny ;#$FD
	sty $8001
	
	lda #$00
	sta $A001 ; disable PRG RAM
	
	jsr api_sound_reset
	jsr oam_clear
	jsr oam_swap

	lda #$20
	sta PPU_ADDR
	lda #$00
	sta PPU_ADDR
	ldx #$00
	.nametable_loop_0:
		lda screen_jax, x
		sta PPU_DATA
		inx
		bne .nametable_loop_0

	.nametable_loop_1:
		lda screen_jax+$100, x
		sta PPU_DATA
		inx
		bne .nametable_loop_1

	.nametable_loop_2:
		lda screen_jax+$200, x
		sta PPU_DATA
		inx
		bne .nametable_loop_2

	.nametable_loop_3:
		lda screen_jax+$300, x
		sta PPU_DATA
		inx
		bne .nametable_loop_3

	;setup palette	
	lda #$3F
	sta PPU_ADDR
	lda #$00
	sta PPU_ADDR
	ldy #$00
	.palette_loop:
		lda palette, y
		sta PPU_DATA
		iny
		cpy #$20
		bne .palette_loop
		
	jsr api_waitblank
	
	lda #$00
	sta frame_count
	sta animation_frame
	sta chr_select
	sta test_delta
	
	lda #$00
	sta PPU_ADDR
	sta PPU_SCROLL
	sta PPU_SCROLL
	sta PPU_ADDR
	sta <nmi_toggle

	lda #GAME_PPU_CTRL
	sta PPU_CTRL
	lda #GAME_PPU_MASK
	sta PPU_MASK	

main_infin:
	lda <nmi_toggle
	beq main_infin
	lda #$00
	sta <nmi_toggle
	jsr oam_swap
	
	inc frame_count
	lda frame_count
	and #$01
	bne .frame_odd
		;0
		ldx #$00
		stx $8000
		ldy #$00
		sty $8001
		;1
		inx
		stx $8000
		iny
		iny
		sty $8001
		jmp .frame_ok
	.frame_odd:
		;0
		ldx #$00
		stx $8000
		ldy #$04
		sty $8001
		;1
		inx
		stx $8000
		iny
		iny
		sty $8001
	
	.frame_ok:
	
	;jsr do_eye_animation
	;jsr do_palette_fade

	;jsr joy_control
	;jsr control_cursor
	;jsr draw_red_eye
	jmp main_infin

;--------------------------------------------------------------------	
joy_control:
	jsr joy_read
	
	lda joy_xor+0
	and joy_raw+0
	sta <tmp10
	
	lda <tmp10
	and #JOY_UP
	beq joy_no_up
		dec cursor
		lda cursor
		cmp #$FF
		bne joy_no_up_ok
			lda #$02
			sta cursor
		joy_no_up_ok:
		jsr cursor_target_update
		jsr api_sound_bleep
	joy_no_up:

	lda <tmp10
	and #JOY_DOWN
	beq joy_no_down
		inc cursor
		lda cursor
		cmp #$03
		bne joy_no_down_ok
			lda #$00
			sta cursor
		joy_no_down_ok:
		jsr cursor_target_update
		jsr api_sound_bleep
	joy_no_down:
	
	lda <tmp10
	and #(JOY_START)
	beq joy_no_start
		jmp joy_start
	joy_no_start:
	
	rts
	
;--------------------------------------------------------------------
cursor_target_update:
	lda cursor
	asl a
	asl a
	asl a
	asl a
	;clc
	;adc cursor
	;clc
	;adc cursor
	;clc
	;adc cursor
	;clc
	;adc cursor
	;clc
	;adc cursor
	;clc
	;adc cursor
	
	clc
	adc #$66
	sta cursor_target
	rts
	
;--------------------------------------------------------------------			
control_cursor:
	lda cursor_target
	cmp cursor_smooth
	beq control_cursor_done
	bcs control_cursor_inc
		dec cursor_smooth
		dec cursor_smooth
	jmp control_cursor_done
	control_cursor_inc:
		inc cursor_smooth
		inc cursor_smooth
	control_cursor_done:
	
	lda cursor_smooth
	sta sprite_ptr+0	;Y
	lda #$01
	sta sprite_ptr+1	;Index
	lda #$03
	sta sprite_ptr+2	;Flags
	lda #70
	sta sprite_ptr+3	;X
	rts

;--------------------------------------------------------------------			
;draw_red_eye:
;	lda #178
;	sta sprite_ptr+1*4+0	;Y
;	sta sprite_ptr+2*4+0	;Y
;	lda #$01
;	sta sprite_ptr+1*4+1	;Index
;	sta sprite_ptr+2*4+1	;Index
;	lda #$02
;	sta sprite_ptr+1*4+2	;Flags
;	sta sprite_ptr+2*4+2	;Flags
;	lda #113
;	;clc
;	;adc eye_animation, y
;	sta sprite_ptr+1*4+3	;X
;	lda #131
;	;clc
;	;adc eye_animation, y
;	sta sprite_ptr+2*4+3	;X
;	rts

;--------------------------------------------------------------------
;do_eye_animation:
;	inc animation_frame
;	lda animation_frame
;	lsr a
;	lsr a
;	and #31
;	tay
;	lda #$3F
;	sta PPU_ADDR
;	lda #$1A
;	sta PPU_ADDR
;	lda eye_animation_palette, y
;	sta PPU_DATA
;
;	rts

;--------------------------------------------------------------------
eye_animation_palette:
	.db $06, $06, $06, $06, $06, $06, $06, $06, $06, $06, $06, $06, $06, $06, $06, $06
	.db $06, $06, $06, $06, $06, $06, $06, $16, $26, $36, $30, $36, $26, $16, $06, $06

;--------------------------------------------------------------------
;eye_animation:
;	.db $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
;	.db $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
;	.db $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
;	.db $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
;	.db $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
;	.db $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01
;	.db $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $00, $00, $00, $00, $00, $00
;	.db $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
;	.db $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
;	.db $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
;	.db $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
;	.db $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
;	.db $ff, $ff, $ff, $ff, $ff, $ff, $ff, $ff, $ff, $ff, $ff, $ff, $ff, $ff, $ff, $ff
;	.db $ff, $ff, $ff, $ff, $ff, $ff, $ff, $ff, $ff, $00, $00, $00, $00, $00, $00, $00
;	.db $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
;	.db $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
;	.db $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00

;--------------------------------------------------------------------
joy_start:
	ldy #$00
	launch_game_copy:
		lda code_launch_game, y
		sta ram_opcode, y
		iny
		bne launch_game_copy
	
	jsr api_sound_reset
	lda #$00
	sta	PPU_MASK		
    sta PPU_CTRL
	jmp ram_opcode

;--------------------------------------------------------------------
code_launch_game:
	;request game on mapper
	lda cursor
	clc
	adc #$01
	sta $5FF5

	;Wait 2 frames
    ldx #$02
launch_frame_on:
    bit $2002
    bpl launch_frame_on
launch_frame_off:
    bit $2002
    bmi launch_frame_off
    dex
    bpl launch_frame_on

	jmp [$FFFC] ;execute game
	
;--------------------------------------------------------------------			
draw_mk_sprites_main:
	ldy animation_frame
	lda [tmp8],y
	sta <addr0+0
	lda [tmpA],y
	sta <addr0+1
	
	ldx <sprite_pos
	ldy #$00
	lda [addr0], y
	sta <tmp0
	.oma_loop:
		iny
		lda [addr0], y
		clc
		adc <tmp11
		sta sprite_ptr, x
		inx
		
		stx <tmp1
			lsr a
			lsr a
			lsr a
			lsr a
			tax
			inc sprite_limit, x
		ldx <tmp1
		
		iny
		lda [addr0], y
		clc
		adc <tmp12
		sta sprite_ptr, x
		inx
		
		iny
		lda [addr0], y
		sta sprite_ptr, x
		inx
		
		iny
		lda [addr0], y
		clc
		adc <tmp10
		sta sprite_ptr, x
		
		dex
		dex
		dex
		
		dex
		dex
		dex
		dex
	
	dec <tmp0
	bne .oma_loop
	stx <sprite_pos
	
	rts
	
;--------------------------------------------------------------------			
draw_mk_sprites_detailed:
	ldy animation_frame
	lda [tmp8],y
	sta <addr0+0
	lda [tmpA],y
	sta <addr0+1
	
	ldx <sprite_pos
	ldy #$00
	lda [addr0], y
	sta <tmp0
	.oma_loop:
		iny
		lda [addr0], y
		clc
		adc <tmp11
		sta sprite_ptr, x
		
		stx <tmp1
			lsr a
			lsr a
			lsr a
			lsr a
			sta <tmp3
			tax
			lda sprite_limit, x
			cmp #$08
			bcc .draw_sprite
				ldx <tmp1
				lda #$F8
				sta sprite_ptr, x
				iny
				iny
				iny
				dec <tmp0
				bne .oma_loop
				jmp .oma_loop_end
			.draw_sprite:
			
			;Mark sprite limit
			ldx <tmp3
			inc sprite_limit, x
		ldx <tmp1
		
		inx

		iny
		lda [addr0], y
		clc
		adc <tmp12
		sta sprite_ptr, x
		inx
		
		iny
		lda [addr0], y
		sta sprite_ptr, x
		inx
		
		iny
		lda [addr0], y
		clc
		adc <tmp10
		sta sprite_ptr, x
		
		dex
		dex
		dex
		
		dex
		dex
		dex
		dex
	
	dec <tmp0
	bne .oma_loop
.oma_loop_end:
	stx <sprite_pos
	rts


;--------------------------------------------------------------------	
palette:
    .db $0f, $00, $10, $30
    .db $0f, $17, $27, $37
    .db $0f, $04, $14, $24
	.db $0f, $04, $14, $24

    .db $0f, $00, $10, $30
    .db $0f, $17, $27, $37
    .db $0f, $04, $14, $24
	.db $0f, $04, $14, $24
	
;--------------------------------------------------------------------
palette_fade:
	.db $3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F
	.db $3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F
	.db $3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F
	.db $3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F
	.db $00,$01,$02,$03,$04,$05,$06,$07,$08,$09,$0A,$0B,$0C,$0D,$0E,$0F
	.db $10,$11,$12,$13,$14,$15,$16,$17,$18,$19,$1A,$1B,$1C,$1D,$1E,$1F
	.db $20,$21,$22,$23,$24,$25,$26,$27,$28,$29,$2A,$2B,$2C,$2D,$2E,$2F
	.db $30,$31,$32,$33,$34,$35,$36,$37,$38,$39,$3A,$3B,$3C,$3D,$3E,$3F
	
;--------------------------------------------------------------------
main_nmi:
	inc <nmi_toggle
	rti

;--------------------------------------------------------------------
main_irq:
	rti

sprite_limit_clear:
	lda #$00
	ldy #$0F
	.loop:
		sta sprite_limit+$00, y
		sta sprite_limit+$10, y
		sta sprite_limit+$20, y
		sta sprite_limit+$30, y
		sta sprite_limit+$40, y
		sta sprite_limit+$50, y
		sta sprite_limit+$60, y
		sta sprite_limit+$70, y
		sta sprite_limit+$80, y
		sta sprite_limit+$90, y
		sta sprite_limit+$A0, y
		sta sprite_limit+$B0, y
		sta sprite_limit+$C0, y
		sta sprite_limit+$D0, y
		sta sprite_limit+$E0, y
		sta sprite_limit+$F0, y
		dey
	bpl .loop
	rts
