;------------------------------------------------------------------
api_ram_clear:
	lda #$00
	tax
api_ram_clear_loop:
    sta $00,x
    ;sta $100,x
    sta $200,x
    sta $300,x
    sta $400,x
    sta $500,x
    sta $600,x
    sta $700,x
    inx
    bne api_ram_clear_loop	
	rts
	
;------------------------------------------------------------------	
api_waitblank:
	bit $2002
api_waitblank_loop:
	lda $2002
	bpl api_waitblank_loop
    rts

;--------------------------------------------------------------------
;A - value
;X - count low part
;Y - count high part
api_ppu_fill:
api_ppu_fill_loop:
	sta PPU_DATA
	dex
	bne api_ppu_fill_loop
	dey
	bne api_ppu_fill_loop
	rts
	
;--------------------------------------------------------------------
api_draw_text:
  ldy #$00
api_draw_text_loop:
	lda [addr0], y
	beq api_draw_text_done
	sta PPU_DATA
	iny
	bne api_draw_text_loop
api_draw_text_done:
  rts
  
;--------------------------------------------------------------------
api_detect_tv_system:
  ;Detect TV system
  ldx #0
  lda #0
  sta <tmp0

detect_tv_wait_sync_1:
  ldy PPU_STATUS
  bpl detect_tv_wait_sync_1

  ; NTSC $1B2
  ; PAL  #1D3
  ; Dendy $18B
detect_tv_wait_sync_2:
  inx
  adc <tmp0
  ldy PPU_STATUS
  bpl detect_tv_wait_sync_2
  
  txa
  sta <tv_system_counter
  and #$F0
  cmp #$80
  beq tv_system_dendy
  cmp #$B0
  beq tv_system_ntsc
  cmp #$D0
  beq tv_system_pal

  ;Default
  lda #$00
  jmp tv_system_done
tv_system_dendy:
  lda #$02
  sta <tv_system_type
  lda #$00
  jmp tv_system_done
tv_system_ntsc:
  lda #$00
  sta <tv_system_type
  jmp tv_system_done
tv_system_pal:
  lda #$01
  sta <tv_system_type
  lda #$10

tv_system_done:
  sta <tv_system_type

  rts

;--------------------------------------------------------------------
joy_read:
	;process input
	lda #$01
	sta CTRL_PORT1
	sta joy_raw
	sta joy_raw+1
	
	lda #$0
	sta CTRL_PORT1
loop_read_joy1:
	lda CTRL_PORT1
	lsr a
	rol joy_raw
	bcc loop_read_joy1

	lda #$0
loop_read_joy2:
	lda CTRL_PORT2
	lsr a
	rol joy_raw+1
	bcc loop_read_joy2
  
	ldy #$01
loop_joy_xor:
    lda joy_raw,y
    eor joy_prev,y
    sta joy_xor,y
    lda joy_raw,y
    sta joy_prev,y
    dey
	bpl loop_joy_xor
	rts

;--------------------------------------------------------------------
oam_clear:
	lda #$FC
	sta <sprite_pos
	lda #$f8
	ldx #$00
	oam_clear_loop:
		sta sprite_ptr, x
		inx
		inx
		inx
		inx
		bne oam_clear_loop
	rts

;--------------------------------------------------------------------
oam_swap:
	lda #high(sprite_ptr)
	sta PPU_OAM_DMA
	rts

;--------------------------------------------------------------------
;addr0 - src address
;tmpF - used
chr_unpack:
    ldy #$00

chr_unpack_decode_loop:
	lda [addr0],y
	beq chr_unpack_done
	tax
	and #$C0
	bne chr_unpack_equals_bytes

chr_unpack_raw:
	  stx <tmpF
	  jsr chr_unpack_increment_address
	  chr_unpack_raw_loop:	;Fast Copy loop
		lda [addr0], y
	    sta PPU_DATA
		iny
	    dex
		bne chr_unpack_raw_loop

      ;increment address
	  lda <tmpF
	  clc
	  adc <addr0
	  sta <addr0
	  lda #$00
	  adc <addr0+1
	  sta <addr0+1
	  ldy #$00
	  beq chr_unpack_decode_loop
	  ;jmp chr_unpack_decode_loop

chr_unpack_equals_bytes:
      cmp #$40
	  beq chr_unpack_eq_00
	  cmp #$80
	  beq chr_unpack_eq_ff

      txa
      and #$3F
	  tax
	  jsr chr_unpack_increment_address
	  lda [addr0], y
	  chr_unpack_fill_loop:
	    sta PPU_DATA
		dex
	  bne chr_unpack_fill_loop
	  jsr chr_unpack_increment_address
	  jmp chr_unpack_decode_loop
	  
chr_unpack_eq_00:
      txa
      and #$3F
	  tax
	  lda #$00
      chr_unpack_eq_00_loop:	
        sta PPU_DATA
		dex
	  bne chr_unpack_eq_00_loop
	  jsr chr_unpack_increment_address
	  jmp chr_unpack_decode_loop
	 
chr_unpack_eq_ff:
      txa
      and #$3F
	  tax
	  lda #$FF
      chr_unpack_eq_ff_loop:	
        sta PPU_DATA
		dex
	  bne chr_unpack_eq_ff_loop
	  jsr chr_unpack_increment_address
	  jmp chr_unpack_decode_loop
	  
chr_unpack_done:
    rts
	
chr_unpack_increment_address:
    inc <addr0
	bne chr_unpack_increment_address_done
	inc <addr0+1
chr_unpack_increment_address_done:
    rts

;--------------------------------------------------------------------
api_sound_reset:
    lda #0
    sta $4000
    sta $4001
    sta $4002
    sta $4003
    sta $4004
    sta $4005
    sta $4006
    sta $4007
    sta $4008
    sta $4009
    sta $400A
    sta $4010
    sta $4011
    sta $4012
    sta $4013
    ldx #$40
    stx $4017 ; disable APU frame IRQ
    rts

;--------------------------------------------------------------------
;Retro sound
api_sound_bleep:
    lda #%00000001
    sta $4015
    lda #%10000111
    sta $4000 
    lda #%10001001
    sta $4001
    lda #%11110000
    sta $4002
    lda #%00001000
    sta $4003
    rts
