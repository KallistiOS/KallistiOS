

	;;
	;; tetris.s
	;;
	;; version 1.3
	;;
	;; by <marcus@idonex.se>
	;; 
	;; Need aslc86k v1.6+ to assemble this.
	;;

	
	.include "sfr.i"


	;; Game variables

piece_x = $30			; X position of current piece (0-10)
piece_y = $31			; Y position of current piece (0-18)
piece_n = $32			; shape number of current piece (0-6)
piece_r = $33			; rotation of current piece (0-3)
piece_i0 = $34			; left half bitmap for current piece
piece_i1 = $35			; right half bitmap for current piece
gotkeys = $36			; keys currently being pressed
time = $37			; time until piece falls down one step
speed = $38			; current game speed
scorelo = $39			; low byte of player score
scorehi = $3a			; high byte of player score
keyinhib = $3b			; keys to inhibit at next scan
seed = $3c			; random seed
			
hitmap = $3e ; - $69		; obstacle presence map, 2 bytes/line,
				; 22 lines
		

	;; Reset and interrupt vectors
	
	.org	0

	jmpf	start

	.org	$3

	jmp	nop_irq

	.org	$b

	jmp	nop_irq
	
	.org	$13

	jmp	nop_irq

	.org	$1b

	jmp	t1int
	
	.org	$23

	jmp	nop_irq

	.org	$2b

	jmp	nop_irq
	
	.org	$33

	jmp	nop_irq

	.org	$3b

	jmp	nop_irq

	.org	$43

	jmp	nop_irq

	.org	$4b

	clr1	p3int,0
	clr1	p3int,1
nop_irq:
	reti


	.org	$130
	
t1int:
	push	ie
	clr1	ie,7
	not1	ext,0
	jmpf	t1int
	pop	ie
	reti

		
	.org	$1f0

goodbye:	
	not1	ext,0
	jmpf	goodbye


	;; Header
	
	.org	$200

	.byte	"Tiny Tetris     "
	.byte	"Mini VMU Tetris by marcus       "

	;; Icon header
	
	.org	$240

	.word	2,10		; Two frames

	;; Icon palette
	
	.org	$260

	.word	$0000, $fcfc, $f0a0, $f0f0, $fccf, $f00a, $f00f, $ffff
	.word	$ffff, $ffff, $ffff, $ffff, $ffff, $ffff, $ffff, $ffff

	;; Icon
	
	.org	$280

	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$01,$11,$11,$11,$11,$11,$10,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$01,$22,$22,$31,$22,$22,$30,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$01,$22,$22,$31,$22,$22,$30,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$01,$22,$22,$31,$22,$22,$30,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$01,$22,$22,$31,$22,$22,$30,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$01,$33,$33,$31,$33,$33,$30,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$01,$11,$11,$11,$11,$11,$10,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$01,$22,$22,$31,$22,$22,$30,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$01,$22,$22,$31,$22,$22,$30,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$01,$22,$22,$31,$22,$22,$30,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$01,$22,$22,$31,$22,$22,$30,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$01,$33,$33,$31,$33,$33,$30,$00,$00,$00
	.byte	$00,$00,$00,$04,$44,$44,$40,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$04,$55,$55,$60,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$04,$55,$55,$60,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$04,$55,$55,$60,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$04,$55,$55,$60,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$04,$66,$66,$60,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$04,$44,$44,$44,$44,$44,$44,$44,$44,$40,$00,$00,$00
	.byte	$00,$00,$00,$04,$55,$55,$64,$55,$55,$64,$55,$55,$60,$00,$00,$00
	.byte	$00,$00,$00,$04,$55,$55,$64,$55,$55,$64,$55,$55,$60,$00,$00,$00
	.byte	$00,$00,$00,$04,$55,$55,$64,$55,$55,$64,$55,$55,$60,$00,$00,$00
	.byte	$00,$00,$00,$04,$55,$55,$64,$55,$55,$64,$55,$55,$60,$00,$00,$00
	.byte	$00,$00,$00,$04,$66,$66,$64,$66,$66,$64,$66,$66,$60,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00

	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$01,$11,$11,$11,$11,$11,$10,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$01,$22,$22,$31,$22,$22,$30,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$01,$22,$22,$31,$22,$22,$30,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$01,$22,$22,$31,$22,$22,$30,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$01,$22,$22,$31,$22,$22,$30,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$01,$33,$33,$31,$33,$33,$30,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$04,$44,$44,$41,$11,$11,$11,$11,$11,$10,$00,$00,$00
	.byte	$00,$00,$00,$04,$55,$55,$61,$22,$22,$31,$22,$22,$30,$00,$00,$00
	.byte	$00,$00,$00,$04,$55,$55,$61,$22,$22,$31,$22,$22,$30,$00,$00,$00
	.byte	$00,$00,$00,$04,$55,$55,$61,$22,$22,$31,$22,$22,$30,$00,$00,$00
	.byte	$00,$00,$00,$04,$55,$55,$61,$22,$22,$31,$22,$22,$30,$00,$00,$00
	.byte	$00,$00,$00,$04,$66,$66,$61,$33,$33,$31,$33,$33,$30,$00,$00,$00
	.byte	$00,$00,$00,$04,$44,$44,$44,$44,$44,$44,$44,$44,$40,$00,$00,$00
	.byte	$00,$00,$00,$04,$55,$55,$64,$55,$55,$64,$55,$55,$60,$00,$00,$00
	.byte	$00,$00,$00,$04,$55,$55,$64,$55,$55,$64,$55,$55,$60,$00,$00,$00
	.byte	$00,$00,$00,$04,$55,$55,$64,$55,$55,$64,$55,$55,$60,$00,$00,$00
	.byte	$00,$00,$00,$04,$55,$55,$64,$55,$55,$64,$55,$55,$60,$00,$00,$00
	.byte	$00,$00,$00,$04,$66,$66,$64,$66,$66,$64,$66,$66,$60,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00

	
	;; Main program

	.org	$680

start:
	clr1 ie,7
	mov #$a1,ocr
	mov #$09,mcr
	mov #$80,vccr
	clr1 p3int,0
	clr1 p1,7
	mov #$ff,p3

	clr1 psw,1		; Get random seed from current minute and
	ld $1c			; second system variables
	xor $1d
	set1 psw,1
	st seed
	
	set1 ie,7

	call clrscr

	mov #<introscreen,trl
	mov #>introscreen,trh
	call setscr

waitstart:
	call getkeys
	and #$30
	bnz waitstart

	mov #<mainscreen,trl
	mov #>mainscreen,trh
	call setscr
	mov #0,acc
	st scorelo
	st scorehi
	call addscore
	call init_hitmap
	mov #100,speed
	mov #0,keyinhib

newpiece:
	mov #5,piece_x
	mov #0,piece_y
	call random
	st c
	mov #0,acc
	mov #7,b
	div
	ld b
	st piece_n
	mov #0,piece_r
	ld speed
	st time
	
	call getpiece
	call checkpiece
	bnz gameover
	call drawpiece

gameloop:
	call kcontrol
	bnz fall
	dbnz time,gameloop
fall:	
	ld speed
	st time
	ld piece_y
	be #18,bottom
	inc piece_y
	call checkpiece
	dec piece_y
	bnz bottom
	call drawpiece
	inc piece_y
	call drawpiece
	br gameloop
bottom:
	mov #4,acc
	call addscore
	call etchpiece
	call checklines
	br newpiece

gameover:	
	mov #<go_sign,trl
	mov #>go_sign,trh
	call show_go
	br waitstart
	
	
kcontrol:
	call getkeys
	push acc
	or keyinhib
	st gotkeys
	pop acc
	xor #$ff
	and #$30
	st keyinhib

	bp gotkeys,2,noleft
	ld piece_x
	bz noleft

	dec piece_x
	call checkpiece
	inc piece_x
	bnz noleft
	call drawpiece
	dec piece_x
	call drawpiece

noleft:
	bp gotkeys,3,noright
	ld piece_x
	be #10,noright

	inc piece_x
	call checkpiece
	dec piece_x
	bnz noright
	call drawpiece
	inc piece_x
	call drawpiece

noright:
	bp gotkeys,4,norot

	push piece_r
	push piece_i0
	push piece_i1
	ld piece_r
	inc acc
	and #3
	st piece_r
	call getpiece
	call checkpiece
	pop piece_i1
	pop piece_i0
	bnz norot0
	pop acc
	call drawpiece
	call getpiece
	call drawpiece
	push piece_r
norot0:	pop piece_r
norot:	mov #0,acc
	bp gotkeys,1,nodown
	inc acc
nodown:	ret

	
		
getkeys:
	bp p7,0,quit
	ld p3
	bn acc,6,quit
	bn acc,7,sleep
	ret

quit:
	jmp goodbye
	
sleep:
	bn p3,7,sleep	; Wait for SLEEP to be depressed
	mov #0,vccr	; Blank LCD
sleepmore:
	set1 pcon,0	; Enter HALT mode
	bp p7,0,quit	; Docked?
	bp p3,7,sleepmore	; No SLEEP press yet
	mov #$80,vccr	; Re-enable LCD
waitsleepup:
	bn p3,7,waitsleepup
	br getkeys


	;; Function:	addscore
	;;
	;; Adds a number to player score, and displays current score
	;;
	;; Inputs:
	;;   acc = score increment

addscore:
	push acc
	push b
	push c
	push 2
	push xbnk
	add scorelo
	st scorelo
	st c
	ld scorehi
	addc #0
	st scorehi
	mov #1,xbnk
	call drawdig2
	mov #0,xbnk
	call drawdig2
	pop xbnk
	pop 2
	pop c
	pop b
	pop acc
	ret


	;; Function:	drawdig2
	;;
	;; Draws two digits for the score indicator
	;;
	;; Inputs:
	;;   acc,c = a 16-bit number.  The two least significant
	;;           digits will be displayed.
	;;   xbnk  = XRAM bank in which to draw the digits
	;; 
	;; Outputs:
	;;   acc,c = The remaining digits (i.e. input number / 100)
	;;
	;; Scratch:
	;;   2 and b will be scratched by this function
	
drawdig2:
	mov #$c5,2
	mov #10,b
	div
	call drawdig
	mov #$85,2
	mov #10,b
	div
	; fallthrough to drawdig...

	
	;; Function:	drawdig
	;;
	;; Draws a single digit for the score indicator
	;;
	;; Inputs:
	;;   b    = digit (0-9)
	;;   2    = position in XRAM where to draw the digit
	;;   xbnk = XRAM bank in which to draw the digit
	;;
	;; Scratch:
	;;   b will be scratched by this function
		
drawdig:
	clr1 ocr,5
	push acc
	push trh
	push trl
	push 2
	mov #<dig_table,trl
	mov #>dig_table,trh
	ld b
	clr1 psw,cy
	rolc
	clr1 psw,cy
	rolc
	clr1 psw,cy
	rolc
	st b
.ddloop:ld b
	ldc 
	st @R2
	inc b
	ld 2
	add #6
	st 2
	ld b
	ldc 
	st @R2
	inc b
	ld 2
	add #10
	st 2
	ld b
	and #7
	bnz .ddloop
	pop 2
	pop trl
	pop trh
	pop acc
	set1 ocr,5
	ret


	;; Function:	getpiece
	;;
	;; Gets a prerotated piece bitmap (4x4 pixels) from ROM
	;;
	;; Inputs:
	;;   piece_n = piece number (0-6)
	;;   piece_r = piece rotation (0-3)
	;;
	;; Outputs:
	;;   piece_i0 = left half bitmap
	;;   piece_i1 = right half bitmap
	
getpiece:			
	push trh
	push trl
	mov #<piece_table,trl
	mov #>piece_table,trh
	ld piece_n
	rol
	rol
	or piece_r
	rol
	push acc
	ldc
	st piece_i0
	pop acc
	inc acc
	ldc
	st piece_i1
	pop trl
	pop trh
	ret


	;; Function:	drawpiece
	;;
	;; Draws/removes the current piece (XOR mode)
	;;
	;; Inputs:	
	;;   piece_i0 = left half bitmap
	;;   piece_i1 = right half bitmap
	;;   piece_x  = x coordinate (0-10)
	;;   piece_y  = y coordinate (0-18)
	
drawpiece:
	clr1 ocr,5
	push acc
	push b
	push c
	push xbnk
	push 2
	push 3
	push trl
	push trh
	ld piece_i0
	st trl
	ld piece_i1
	st trh
	ld piece_x
	add #17
	st b
	clr1 psw,cy
	rorc
	clr1 psw,cy
	rorc
	clr1 psw,cy
	rorc
	bn piece_y,0,.even
	add #6
.even:	st 2
	ld piece_y
	add #8
	mov #0,xbnk
	bn acc,4,.upper
	inc xbnk
.upper:	and #$0e
	clr1 psw,cy
	rolc
	clr1 psw,cy
	rolc
	clr1 psw,cy
	rolc
	add 2
	or #$80
	st 2
	ld b
	and #7
	st b
	inc b
	mov #1,acc
.getbit:ror
	dbnz b,.getbit
	st b
	mov #4,3
.doall:	mov #4,c
	push 2
	push xbnk
.docol:	ld trh
	rorc
	st trh
	ld trl
	rorc
	st trl
	bn psw,cy,.transp
	ld b
	xor @R2
	st @R2
.transp:ld 2
	and #$f
	be #5,.okeven
	bp psw,cy,.okeven
	ld 2
	add #10
	bn psw,cy,.okodd
	mov #1,xbnk
	or #$80
	br .okodd
.okeven:ld 2
	add #6
.okodd:	st 2
	dbnz c,.docol
	pop xbnk
	pop 2
	ld b
	clr1 psw,cy
	rorc
	st b
	bn psw,cy,.samec
	mov #$80,b
	inc 2
.samec:	dbnz 3,.doall	
	pop trh
	pop trl
	pop 3	
	pop 2
	pop xbnk
	pop c
	pop b
	pop acc
	set1 ocr,5
	ret


	;; Function:	checkpiece
	;;
	;; Checks if the current piece can be placed at the
	;; given location without hitting an obstacle
	;;
	;; Inputs:	
	;;   piece_i0 = left half bitmap
	;;   piece_i1 = right half bitmap
	;;   piece_x  = x coordinate (0-10)
	;;   piece_y  = y coordinate (0-18)
	;;   hitmap   = current obstacle map
	;;
	;; Outputs:
	;;   acc = 1 if obstacle hit, 0 if no obstacle hit
	
checkpiece:
	push b
	push c
	push 1
	push 3
	push trl
	push trh
	ld piece_i0
	st trl
	ld piece_i1
	st trh
	ld piece_y
	clr1 psw,cy
	rolc
	bn piece_x,3,.chleft
	inc acc
.chleft:add #hitmap
	st 1
	ld piece_x
	and #7
	st b
	inc b
	mov #1,acc
.chbit:	ror
	dbnz b,.chbit
	st b
	mov #4,3
.chall:	mov #4,c
	push 1
.chcol:	ld trh
	rorc
	st trh
	ld trl
	rorc
	st trl
	bn psw,cy,.noch
	ld b
	and @R1
	bnz .didhit
.noch:	ld 1
	add #2
	st 1
	dbnz c,.chcol
	pop 1
	ld b
	clr1 psw,cy
	rorc
	st b
	bn psw,cy,.samec
	mov #$80,b
	inc 1
.samec: dbnz 3,.chall
	mov #0,acc
.nohit:	pop trh
	pop trl
	pop 3
	pop 1
	pop c
	pop b
	ret

.didhit:
	pop 1
	mov #1,acc
	br .nohit

	
	;; Function:	etchpiece
	;;
	;; Adds the current piece to the obstacle map
	;;
	;; Inputs:	
	;;   piece_i0 = left half bitmap
	;;   piece_i1 = right half bitmap
	;;   piece_x  = x coordinate (0-10)
	;;   piece_y  = y coordinate (0-18)
	;;   hitmap   = previous obstacle map
	;;
	;; Outputs:
	;;   hitmap = new obstacle map

etchpiece:	
	push b
	push c
	push 1
	push 3
	push trl
	push trh
	ld piece_i0
	st trl
	ld piece_i1
	st trh
	ld piece_y
	clr1 psw,cy
	rolc
	bn piece_x,3,.etleft
	inc acc
.etleft:add #hitmap
	st 1
	ld piece_x
	and #7
	st b
	inc b
	mov #1,acc
.etbit:	ror
	dbnz b,.etbit
	st b
	mov #4,3
.etall:	mov #4,c
	push 1
.etcol:	ld trh
	rorc
	st trh
	ld trl
	rorc
	st trl
	bn psw,cy,.noet
	ld b
	or @R1
	st @R1
.noet:	ld 1
	add #2
	st 1
	dbnz c,.etcol
	pop 1
	ld b
	clr1 psw,cy
	rorc
	st b
	bn psw,cy,.sameet
	mov #$80,b
	inc 1
.sameet:dbnz 3,.etall
	mov #0,acc
	pop trh
	pop trl
	pop 3
	pop 1
	pop c
	pop b
	ret


	;; Function:	checklines
	;;
	;; Removes "full" lines, and increases score and game
	;; speed accordingly
	;;
	;; Inputs:
	;;   hitmap  = previous obstacle map
	;;   scorelo = previous score low byte
	;;   scorehi = previous score high byte
	;;   speed   = previous game speed
	;;
	;; Outputs:
	;;   hitmap  = new obstacle map with lines removed
	;;   scorelo = new score low byte
	;;   scorehi = new score high byte
	;;   speed   = new game speed
	
checklines:
	push acc
	push 0
	push 1
	mov #hitmap+2*19,0
	mov #hitmap+2*19,1
.cklp:	ld @R0
	st @R1
	inc 0
	inc 1
	and @R0
	bne #$ff,.hole
	dec 1
	mov #10,acc
	call addscore
	call incspeed
	br .filld
.hole:	ld @R0
	st @R1
	ld 1
	be 0,.nocpy
	call copyline
.nocpy:	sub #3
	st 1
.filld:	ld 0
	sub #3
	st 0
	bne #hitmap-2,.cklp
	be 1,.ckdne
.cklp2:	mov #$c0,@R1
	inc 1
	mov #$0f,@R1
	ld 1
	call clearline
	sub #3
	st 1
	bne #hitmap-2,.cklp2	
.ckdne:	pop 1
	pop 0
	pop acc
	ret


	;; Function:	incspeed
	;;
	;; Increases game speed, if possibel
	;;
	;; Inputs:
	;;   speed = previous game speed
	;;
	;; Outputs:
	;;   speed = new game speed
	
incspeed:
	push acc
	ld speed
	be #2,.maxsp
	dec acc
	st speed
.maxsp:	pop acc
	ret


	;; Function:	copyline
	;;
	;; Copy one screen line downwards as the pile collapses
	;;
	;; Inputs:
	;;   0   = source line (as pointer into hitmap table)
	;;   acc = destination line (as pointer into hitmap table)
		
copyline:
	clr1 ocr,5
	push 2
	push xbnk
	push trh
	push trl
	push acc
	ld 0
	call calcline
	ld @R2
	and #$1f
	st trl
	inc 2
	ld @R2
	and #$f8
	st trh
	pop acc
	push acc
	call calcline
	ld @R2
	and #$e0
	or trl
	st @R2
	inc 2
	ld @R2
	and #$07
	or trh
	st @R2
	pop acc
	pop trl
	pop trh
	pop xbnk
	pop 2
	set1 ocr,5	
	ret


	;; Function:	clearline
	;;
	;; Clears one screen line as the pile collapses
	;;
	;; Inputs:
	;;   acc = line (as pointer into hitmap table)

clearline:
	clr1 ocr,5
	push acc
	push 2
	push xbnk
	call calcline
	ld @R2
	and #$e0
	st @R2
	inc 2
	ld @R2
	and #$07
	st @R2
	pop xbnk
	pop 2
	pop acc
	set1 ocr,5
	ret


	;; Function:	calcline
	;;
	;; Calculates position in XRAM for a particular screen line
	;;
	;; Inputs:
	;;   acc = line (as pointer into hitmap table)
	;;
	;; Outputs:
	;;   2    = XRAM address
	;;   xbnk = XRAM bank
	
calcline:
	push acc
	mov #$82,2
	sub #hitmap
	clr1 psw,cy
	rorc
	bn acc,0,.cllft
	mov #$88,2
.cllft:	add #8
	mov #0,xbnk
	bn acc,4,.clup
	inc xbnk
.clup:	and #$0e
	clr1 psw,cy
	rolc
	clr1 psw,cy
	rolc
	clr1 psw,cy
	rolc
	add 2
	st 2
	pop acc
	ret


	;; Function:	init_hitmap
	;;
	;; Initializes the obstacle map to only include the
	;; walls and floor
	;;
	;; Outputs:
	;;   hitmap = cleared obstacle map

init_hitmap:
	push 1
	mov #hitmap,1
.ihloop:mov #$c0,@R1
	inc 1
	mov #$0f,@R1
	inc 1
	ld 1
	bne #hitmap+2*22,.ihloop
	mov #$ff,hitmap+2*20
	mov #$ff,hitmap+2*20+1
	pop 1
	ret

	
	;; Function:	clrscr
	;;
	;; Clears the screen

clrscr:	
	clr1 ocr,5
	push acc
	push xbnk
	push 2
	mov #0,xbnk
.cbank:	mov #$80,2
.cloop:	mov #0,@R2
	inc 2
	ld 2
	and #$f
	bne #$c,.cskip
	ld 2
	add #4
	st 2
.cskip:	ld 2
	bnz .cloop
	bp xbnk,0,.cexit
	mov #1,xbnk
	br .cbank
.cexit:	pop 2
	pop xbnk
	pop acc
	set1 ocr,5
	ret

	
	;; Function:	setscr
	;;
	;; Copies a predefined full-screen image to the screen
	;;
	;; Inputs:
	;;   trl = low byte of predefined screen ROM address
	;;   trh = high byte of predefined screen ROM address
	
setscr:
	clr1 ocr,5
	push acc
	push xbnk
	push c
	push 2
	mov #$80,2
	xor acc
	st xbnk
	st c
.sloop:	ldc
	st @R2
	inc 2
	ld 2
	and #$f
	bne #$c,.sskip
	ld 2
	add #4
	st 2
	bnz .sskip
	inc xbnk
	mov #$80,2
.sskip:	inc c
	ld c
	bne #$c0,.sloop
	pop 2
	pop c
	pop xbnk
	pop acc
	set1 ocr,5
	ret


	;; Function:	show_go
	;;
	;; Draws a 16x13 pixel sign in the middle of the screen
	;;
	;; Inputs:
	;;   trl = low byte of sign bitmap ROM address
	;;   trh = high byte of sign bitmap ROM address

show_go:
	clr1 ocr,5
	push xbnk
	push 2
	mov #0,xbnk
	mov #$e2,2
	mov #0,c
	ld c
.shlp:	ldc
	st @R2
	inc c
	inc 2
	ld c
	ldc
	st @R2
	inc c
	ld 2
	add #5
	bp c,1,.shodd
	add #4
	bn psw,cy,.shodd
	add #$80
	mov #1,xbnk
.shodd:	st 2
	ld c
	bne #26,.shlp
	pop 2
	pop xbnk
	set1 ocr,5
	ret


	;; Function:	random
	;;
	;; Generates a pseudo-random value in the range 0-255
	;;
	;; Inputs:
	;;   seed = previous random seed
	;;
	;; Outputs:
	;;   seed = new random seed
	;;   acc  = generated random value

random:
	push b
	push c
	ld seed
	st b
	mov #$4e,acc
	mov #$6d,c
	mul
	st b
	ld c
	add #$39
	st seed
	ld b
	addc #$30
	pop c
	pop b
	ret
	

	;; Intro screen

introscreen:
	.byte %11011011,%01101101,%10110110,%11000011,%01101100,%00110110
	.byte %11011011,%01101101,%10110110,%11000011,%01101100,%00110110
	.byte %00000000,%00000000,%00000000,%00000000,%00000000,%00000000
	.byte %00011000,%01100000,%00000110,%00011000,%01101101,%10000000
	.byte %00011000,%01100000,%00000110,%00011000,%01101101,%10000000
	.byte %00000000,%00000000,%00000000,%00000000,%00000000,%00000000
	.byte %00011000,%01101100,%00000110,%00000011,%01101100,%00110000
	.byte %00011000,%01101100,%00000110,%00000011,%01101100,%00110000
        .byte %00000000,%00000000,%00000000,%00000000,%00000000,%00000000
	.byte %00011000,%01100000,%00000110,%00011000,%01101100,%00000110
	.byte %00011000,%01100000,%00000110,%00011000,%01101100,%00000110
	.byte %00000000,%00000000,%00000000,%00000000,%00000000,%00000000
	.byte %00011000,%01101101,%10000110,%00011000,%01101101,%10110000
	.byte %00011000,%01101101,%10000110,%00011000,%01101101,%10110000
	.byte %00000000,%00000000,%00000000,%00000000,%00000000,%00000000
	.byte %00000000,%00000000,%00000000,%00000000,%00000000,%00000000
        .byte %00000001,%10010100,%00101001,%10010100,%10010100,%11000000
	.byte %00000001,%01010100,%01010100,%01011001,%01010101,%00000000
	.byte %00000001,%10001000,%01010100,%11010001,%00010100,%10000000
	.byte %00000001,%01001000,%01010101,%01010001,%01010100,%01000000
	.byte %00000001,%10001000,%01010100,%11010000,%10001101,%10000000
	.byte %00000000,%00000000,%00000000,%00000000,%00000000,%00000000
	.byte %00000000,%00000000,%00000000,%00000000,%00000000,%00000000
	.byte %00001111,%11111111,%11111111,%11111111,%11111111,%11110000
        .byte %00001000,%00000000,%00000000,%00000000,%00000000,%00010000
	.byte %00001011,%00110011,%10011001,%10000011,%00000011,%10010000
	.byte %00001010,%10101010,%00100010,%00000100,%10010010,%01010000
	.byte %00001011,%00110011,%00010001,%00000111,%10111011,%10010000
	.byte %00001010,%00101010,%00001000,%10000100,%10010010,%01010000
	.byte %00001010,%00101011,%10110011,%00000100,%10000011,%10010000
	.byte %00001000,%00000000,%00000000,%00000000,%00000000,%00010000
	.byte %00001111,%11111111,%11111111,%11111111,%11111111,%11110000


	;; Main game screen

mainscreen:	
	.byte %00000000,%00000000,%00000000,%00000000,%00000000,%00000000
	.byte %00100110,%00000000,%00000000,%00000000,%00000000,%00000000
	.byte %01001001,%00000000,%00000000,%00000000,%00000000,%00000000
	.byte %01001001,%00000000,%00000000,%00000000,%00000000,%00000000
	.byte %01001001,%00000000,%00000000,%00000000,%00000000,%00000000
	.byte %00110010,%00000000,%00000000,%00000000,%00000000,%00000000
	.byte %00000000,%00000000,%00000000,%00000000,%00000000,%00000000
	.byte %01111111,%00000000,%00000000,%00000000,%00000000,%00000000
	.byte %00000000,%00000001,%11100000,%00000111,%10000000,%00000000
	.byte %01111111,%00000001,%00100000,%00000100,%10000000,%00000000
	.byte %01001000,%00000001,%00100000,%00000100,%10000000,%00000000
	.byte %01001100,%00000001,%00100000,%00000100,%10000000,%00000000
	.byte %01001010,%00000001,%00100000,%00000100,%10000000,%00000000
	.byte %00110001,%00000001,%00100000,%00000100,%10000000,%00000000
	.byte %00000000,%00000001,%00100000,%00000100,%10000000,%00000000
	.byte %01000000,%00000001,%00100000,%00000100,%10000000,%00000000
	.byte %01000000,%00000001,%00100000,%00000100,%10000000,%00000000
	.byte %01111111,%00000001,%00100000,%00000100,%10000000,%00000000
	.byte %01000000,%00000001,%00100000,%00000100,%10000000,%00000000
	.byte %01000000,%00000001,%00100000,%00000100,%10000000,%00000000
	.byte %00000000,%00000001,%00100000,%00000100,%10000000,%00000000
	.byte %01000001,%00000001,%00100000,%00000100,%10000000,%00000000
	.byte %01000001,%00000001,%00100000,%00000100,%10000000,%00000000
	.byte %01001001,%00000001,%00100000,%00000100,%10000000,%00000000
	.byte %01001001,%00000001,%00100000,%00000100,%10000000,%00000000
	.byte %01111111,%00000001,%00100000,%00000100,%10000000,%00000000
	.byte %00000000,%00000001,%00100000,%00000100,%10000000,%00000000
	.byte %01000000,%00000001,%00100000,%00000100,%10000000,%00000000
	.byte %01000000,%00000001,%00111111,%11111100,%10000000,%00000000
	.byte %01111111,%00000001,%00000000,%00000000,%10000000,%00000000
	.byte %01000000,%00000001,%00000000,%00000000,%10000000,%00000000
	.byte %01000000,%00000001,%11111111,%11111111,%10000000,%00000000


	;; Game Over sign
	
go_sign:	
	.byte %00000000,%00000000
	.byte %01000100,%10101110
	.byte %10001010,%11101000
	.byte %11101110,%10101100
	.byte %10101010,%10101000
	.byte %01001010,%10101110
	.byte %00000000,%00000000
	.byte %01001010,%11101100
	.byte %10101010,%10001010
	.byte %10101010,%11001100
	.byte %10101010,%10001010
	.byte %01000100,%11101010
	.byte %00000000,%00000000


	;; Piece table

piece_table:
	.byte $64,$02,$30,$06,$40,$26,$60,$0c
	.byte $62,$04,$60,$03,$20,$46,$c0,$06
	.byte $64,$04,$70,$02,$20,$26,$40,$0e
	.byte $60,$06,$60,$06,$60,$06,$60,$06
	.byte $46,$04,$70,$01,$20,$62,$80,$0e
	.byte $44,$06,$70,$04,$60,$22,$20,$0e
	.byte $22,$22,$00,$0f,$44,$44,$f0,$00


	;; Digit font

dig_table:
	.byte %00111110
	.byte %01100011
	.byte %01100111
	.byte %01101011
	.byte %01110011
	.byte %01100011
	.byte %00111110
	.byte %00000000

	.byte %00011000
	.byte %00111000
	.byte %00011000
	.byte %00011000
	.byte %00011000
	.byte %00011000
	.byte %00111100
	.byte %00000000

	.byte %00111110
	.byte %01100011
	.byte %00000011
	.byte %00111110
	.byte %01100000
	.byte %01100000
	.byte %01111111
	.byte %00000000

	.byte %00111110
	.byte %01100011
	.byte %00000011
	.byte %00001110
	.byte %00000011
	.byte %01100011
	.byte %00111110
	.byte %00000000

	.byte %01100011
	.byte %01100011
	.byte %01100011
	.byte %01111111
	.byte %00000011
	.byte %00000011
	.byte %00000011
	.byte %00000000

	.byte %01111111
	.byte %01100000
	.byte %01100000
	.byte %01111110
	.byte %00000011
	.byte %01100011
	.byte %00111110
	.byte %00000000

	.byte %00111110
	.byte %01100011
	.byte %01100000
	.byte %01111110
	.byte %01100011
	.byte %01100011
	.byte %00111110
	.byte %00000000

	.byte %01111111
	.byte %00000011
	.byte %00000011
	.byte %00000110
	.byte %00001100
	.byte %00001100
	.byte %00001100
	.byte %00000000

	.byte %00111110
	.byte %01100011
	.byte %01100011
	.byte %00111110
	.byte %01100011
	.byte %01100011
	.byte %00111110
	.byte %00000000

	.byte %00111110
	.byte %01100011
	.byte %01100011
	.byte %00111111
	.byte %00000011
	.byte %01100011
	.byte %00111110
	.byte %00000000


	.cnop	0,$200		; pad to an even number of blocks


