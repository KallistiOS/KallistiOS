
! KallistiOS ##version##
!
! arch/dreamcast/hardware/syscall_font.s
!
! Copyright (C) 2024 Andy Barajas
!
! Assembly code for font system calls
!
    .text
    .globl _syscall_font_address
    .globl _syscall_font_lock
    .globl _syscall_font_unlock

!
! uint8_t *syscall_font_address(void);
!
    .align 2
_syscall_font_address:
    mov.l  syscall_font, r0
    mov.l  @r0, r0
    jmp	   @r0
    mov	   #0, r1    ! 0 is FUNC_ROMFONT_ADDRESS

    rts
    nop

!
! int syscall_font_lock(void);
!
    .align 2
_syscall_font_lock:
    mov.l  syscall_font, r0
    mov.l  @r0, r0
    jmp	   @r0
    mov	   #1, r1    ! 1 is FUNC_ROMFONT_LOCK

    rts
    nop

!
! void syscall_font_unlock(void);
!
    .align 2
_syscall_font_unlock:
    mov.l  syscall_font, r0
    mov.l  @r0, r0
    jmp	   @r0
    mov	   #2, r1    ! 2 is FUNC_ROMFONT_UNLOCK

    rts
    nop

! Variables
    .align    4

syscall_font:
    .long    0x8C0000B4    ! VEC_BIOFONT
