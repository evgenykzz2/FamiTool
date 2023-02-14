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
	
	;Cool-Boy hack, need to show black screen
;	lda #%00001010
;	sta $2001
;	ldx #30
;coolbot_start_wait:
;	jsr api_waitblank
;	dex
;	bne coolbot_start_wait		
;	lda #$00
;	sta $2001
;	jsr api_waitblank
	
	;reset mmc3 iqr
    ;lda #$0
    ;sta $C000
    ;sta $C001
    ;sta $E000   ; IRQ disable

	IF MAPPER=4
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
	ENDIF
	
	jsr api_sound_reset
	jsr oam_clear
	jsr oam_swap

	ldx #$00
	stx $8000
	lda #$00
	sta $8001
	inx
	stx $8000
	ora #$02
	sta $8001
	
	lda #low(chi_dale_screen_1)
	sta <addr0+0
	lda #high(chi_dale_screen_1)
	sta <addr0+1
	lda #$20
	sta PPU_ADDR
	lda #$00
	sta PPU_ADDR
	ldy #$00
	ldx #$04
	;lda #$00
	nametable_loop:
		lda [addr0], y
		sta PPU_DATA
		iny
		bne nametable_loop
		inc <addr0+1
		dex
		bne nametable_loop
	
	;setup palette	
	lda #$3F
	sta PPU_ADDR
	lda #$00
	sta PPU_ADDR
	ldy #$00
	palette_loop:
		lda palette, y
		sta PPU_DATA
		iny
		cpy #$10
		bne palette_loop
	
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
	
	;lda #low(audio_sampler_nmi_0)
	;sta <sampler_method_next+0
	;lda #high(audio_sampler_nmi_0)
	;sta <sampler_method_next+1

	;lda #$00
	;sta <sampler_nmi_counter
	;sta <sampler_nmi_screen

	;jmp audio_sampler_main

main_infin:
	lda <nmi_toggle
	beq main_infin
	lda #$00
	sta <nmi_toggle

	jsr oam_swap
	;jsr do_eye_animation
	;jsr do_palette_fade

	lda #$00
	sta PPU_ADDR
	sta PPU_SCROLL
	sta PPU_SCROLL
	sta PPU_ADDR
	
	ldy #$0A
	ldx #$00
	.delay:
		nop
		dex
		bne .delay
		dey
		bne .delay
		;sta $6FFF
		
	lda #$00
	sta PPU_CTRL
	sta PPU_MASK
	
	lda #$3F
	sta PPU_ADDR
	lda #$00
	sta PPU_ADDR
	
	ldx #$00
	.ppu_loop:
		lda main_infin, x
		sta PPU_DATA
		inx
		bne .ppu_loop
	
	lda #GAME_PPU_CTRL
	sta PPU_CTRL
	lda #GAME_PPU_MASK
	sta PPU_MASK

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
palette_bg:
	.db $0f, $00, $10, $30
	.db $0f, $30, $10, $15
	.db $0f, $10, $28, $38
	.db $0f, $30, $36, $26

;--------------------------------------------------------------------			
palette_blink:
	.db $0f, $10, $10, $10
	.db $0f, $10, $10, $10
	.db $0f, $10, $10, $10
	.db $0f, $10, $10, $10

;--------------------------------------------------------------------	
palette:
    .db 0x0f, 0x26, 0x17, 0x36
	.db 0x0f, 0x17, 0x27, 0x10
	.db 0x0f, 0x00, 0x10, 0x36
	.db 0x0f, 0x07, 0x17, 0x27
	
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
do_palette_fade:
	lda <fade_counter
	cmp #$FF
	beq .rts
	
	inc <fade_counter
	lda <fade_counter
	lsr a
	lsr a
	cmp #$04
	bcc .pal_ok
		lda #$04
		rts
	.pal_ok:
	
	;*16
	asl a
	asl a
	asl a
	asl a
	sta <tmp0
	
	lda #high(palette_fade)
	sta <addr0+1

	lda #low(palette_fade)
	clc
	adc <tmp0
	sta <addr0
	lda <addr0+1
	adc #$00
	sta <addr0+1
	
	lda #$3F
	sta PPU_ADDR
	lda #$00
	sta PPU_ADDR
	ldx #$00
	.loop:
		lda palette_bg, x
		tay
		lda [addr0], y
		sta PPU_DATA
		iny
		inx
		cpx #$20
		bne .loop
		
	.rts:
	rts

;--------------------------------------------------------------------
main_nmi:
	inc <nmi_toggle
	rti

;--------------------------------------------------------------------
main_irq:
	rti

;--------------------------------------------------------------------
audio_tick_0_240:
	ldy #$00
	lda [addr0], y
	tax
		lda <audio_raw_ptr+0
		clc
		adc #$01
		sta <audio_raw_ptr+0
		
		lda <audio_raw_ptr+1
		adc #$00
		sta <audio_raw_ptr+1
		cmp #$A0
		bne .no_bank_swap
			lda #$80
			sta <audio_raw_ptr+1
			lda #$06
			sta $8000
			inc <audio_raw_bank
			lda <audio_raw_bank
			sta $8001
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			nop
			nop
			nop
			nop
			nop
			jmp .bank_swap_done			
		.no_bank_swap:
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			lda $0000, x
			cmp <$00
			nop
			nop
		.bank_swap_done:
	stx $4011
	rts

;--------------------------------------------------------------------
;movie_frame_pointers:
;	.db low(frame_000), high(frame_000)


;--------------------------------------------------------------------
audio_sampler_main:
	;1,789,772 cycles per second NTSC
	;1,662,607 cycles per second PAL
	;2500 hz;  715.9088 cycles per sample
	;10000 hz  178.9772 cycles per sample
	
	;Initialize audio pointer
	lda #$00
	sta <audio_raw_bank
	lda #$06
	sta $8000
	lda <audio_raw_bank
	sta $8001
				
	lda #$00
	sta <audio_raw_ptr+0
	lda #$80
	sta <audio_raw_ptr+1
	
	lda #$00
	sta <movie_frame_pointer+0
	lda #$A0
	sta <movie_frame_pointer+1
	
	lda #$00
	sta <movie_ppu_addr+0
	sta <movie_ppu_addr+1

	.wait_nmi:
		lda <nmi_toggle
		beq .wait_nmi
	dec <nmi_toggle
	;line 240

	;nmi code
	sta $4011
	
	;audio-0
	lda #$00
	sta <audio_raw_y
	sta DMC_FREQ
    sta PPU_CTRL
	
	lda <movie_ppu_addr+1
	sta PPU_ADDR
	lda <movie_ppu_addr+0
	sta PPU_ADDR

	lda <$50
	sta PPU_DATA
	
	lda <$51
	sta PPU_DATA
	
	lda <$52
	sta PPU_DATA
	
	lda <$53
	sta PPU_DATA
	
	lda <$54
	sta PPU_DATA
	
	lda <$55
	sta PPU_DATA
	
	lda <$56
	sta PPU_DATA
	
	lda <$57
	sta PPU_DATA
	
	lda <$58
	sta PPU_DATA
	
	lda <$59
	sta PPU_DATA
	
	lda <$5A
	sta PPU_DATA
	
	lda <$5B
	sta PPU_DATA
	
	lda <$5C
	sta PPU_DATA
	
	lda <$5D
	sta PPU_DATA
	
	lda <$5E
	sta PPU_DATA
	
	lda <$5F
	sta PPU_DATA

	iny
	lda [addr0], y
	sta $4011
	
	lda <$60
	sta PPU_DATA
	
	lda <$61
	sta PPU_DATA
	
	lda <$62
	sta PPU_DATA
	
	lda <$63
	sta PPU_DATA
	
	lda <$64
	sta PPU_DATA
	
	lda <$65
	sta PPU_DATA
	
	lda <$66
	sta PPU_DATA
	
	lda <$67
	sta PPU_DATA
	
	lda <$68
	sta PPU_DATA
	
	lda <$69
	sta PPU_DATA
	
	lda <$6A
	sta PPU_DATA
	
	lda <$6B
	sta PPU_DATA
	
	lda <$6C
	sta PPU_DATA
	
	lda <$6D
	sta PPU_DATA
	
	lda <$6E
	sta PPU_DATA
	
	lda <$6F
	sta PPU_DATA
	
	
	
	lda <$60
	sta PPU_DATA
	
	lda <$61
	sta PPU_DATA
	
	lda <$62
	sta PPU_DATA
	
	lda <$63
	sta PPU_DATA
	
	lda <$64
	sta PPU_DATA
	
	lda <$65
	sta PPU_DATA
	
	lda <$66
	sta PPU_DATA
	
	lda <$67
	sta PPU_DATA

	iny
	lda [addr0], y
	sta $4011
	
	
	sta $6FFF
	
	;0..9
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	
	;10..19
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	
	;20..29
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	
	;30..39
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	
	;40..49
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	
	;50..59
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	
	;60..69
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	
	;70..79
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	
	;80..89
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240

	;90..99
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	
	;100..109
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	
	;110..119
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	
	;120..129
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	
	;130..139
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	
	;140..149
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	
	;150..159
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	
	;160..169
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	jsr audio_tick_0_240
	

	
	sta $6FFF
	
	rts

;audio_sampler_delay: ;179 ticks
;	ldx #$1E
;	.loop:
;		dex
;		bne .loop
;	nop
;	jmp audio_sampler_return


