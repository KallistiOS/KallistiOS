/* KallistiOS ##version##

   syscalls.c

   Copyright (C) 2024 Andy Barajas
*/

#include <stdio.h>

#include <dc/syscalls.h>
#include <arch/memory.h>

/*
    This module along with syscall_font.s contains functions for accessing 
    the Dreamcast's system calls.
*/

/*  
    Each system call is performed via an indirect vector (VEC_*). This means 
    that rather than calling a fixed address, a function pointer is fetched 
    from the fixed address and the call goes through this pointer.
*/
#define VEC_SYSINFO       (MEM_AREA_P1_BASE | 0x0C0000B0)
#define VEC_BIOFONT       (MEM_AREA_P1_BASE | 0x0C0000B4)
#define VEC_FLASHROM      (MEM_AREA_P1_BASE | 0x0C0000B8)
#define VEC_MISC_GDROM    (MEM_AREA_P1_BASE | 0x0C0000BC)
#define VEC_GDROM2        (MEM_AREA_P1_BASE | 0x0C0000C0)
#define VEC_SYSTEM        (MEM_AREA_P1_BASE | 0x0C0000E0)

/*
    For each indirect vector there is a number of different functions (FUNC_*) 
    available.  For the VEC_MISC_GDROM vector we have to also supply a super 
    function (SUPER_FUNC_*) to have access to functions (FUNC_*) for either
    MISC or GDROM system calls.
*/

/* SYSINFO functions */
#define FUNC_SYSINFO_INIT     0
#define FUNC_SYSINFO_ICON     2
#define FUNC_SYSINFO_ID       3

/* ROMFONT functions */
#define FUNC_ROMFONT_ADDRESS  0
#define FUNC_ROMFONT_LOCK     1
#define FUNC_ROMFONT_UNLOCK   2

/* FLASHROM functions */
#define FUNC_FLASHROM_INFO    0
#define FUNC_FLASHROM_READ    1
#define FUNC_FLASHROM_WRITE   2
#define FUNC_FLASHROM_DELETE  3

/* MISC/GDROM super functions */
#define SUPER_FUNC_MISC       -1
#define SUPER_FUNC_GDROM      0

/* MISC functions */
#define FUNC_MISC_INIT        0
#define FUNC_MISC_SETVECTOR   1

/* GDROM functions */
#define FUNC_GDROM_SEND_COMMAND     0
#define FUNC_GDROM_CHECK_COMMAND    1
#define FUNC_GDROM_EXEC_SERVER      2
#define FUNC_GDROM_INIT             3
#define FUNC_GDROM_DRIVE_STATUS     4
#define FUNC_GDROM_DMA_CALLBACK     5
#define FUNC_GDROM_DMA_TRANSFER     6
#define FUNC_GDROM_DMA_CHECK        7
#define FUNC_GDROM_ABORT_COMMAND    8
#define FUNC_GDROM_RESET            9
#define FUNC_GDROM_SECTOR_MODE      10
#define FUNC_GDROM_PIO_CALLBACK     11
#define FUNC_GDROM_PIO_TRANSFER     12
#define FUNC_GDROM_PIO_CHECK        13
#define FUNC_GDROM_STUB1            14
#define FUNC_GDROM_STUB2            15

/* SYSTEM functions */
#define FUNC_SYSTEM_RESET           -1
#define FUNC_SYSTEM_BIOS_MENU       1
#define FUNC_SYSTEM_CD_MENU         3

/* A param we submit when functions have unused variables */
#define PARAM_NA  0

/*
    Macros to easily make syscalls depending on the value we have to return.
    
    We use an indirect vector (VEC_*) to create a function pointer and submit
    params to this function using registers r4-r7. r7, "func" in the macros
    below, is always the function (FUNC_*) selector. Sometimes there is also
    a superfunction (SUPER_FUNC_*) to be selected by register r6. The ROMFONT
    syscalls are a bit peculiar by using r1 instead of r7 to select a function
    so they had to be moved to their own file syscall_font.s.
*/

/* Sets a value of a specified 'type' */
#define MAKE_SYSCALL_SET(vec, func, r4, r5, r6, result, type) do { \
    uintptr_t *syscall_ptr = (uintptr_t *)(vec); \
    type (*syscall)() = (type (*)())(*syscall_ptr); \
    result = syscall((r4), (r5), (r6), (func)); \
} while(0)

/* Returns an int */
#define MAKE_SYSCALL_INT(vec, func, r4, r5, r6) do { \
    uintptr_t *syscall_ptr = (uintptr_t *)(vec); \
    int (*syscall)() = (int (*)())(*syscall_ptr); \
    return syscall((r4), (r5), (r6), (func)); \
} while(0)

/* Returns nothing */
#define MAKE_SYSCALL_VOID(vec, func, r4, r5, r6) do { \
    uintptr_t *syscall_ptr = (uintptr_t *)(vec); \
    void (*syscall)() = (void (*)())(*syscall_ptr); \
    syscall((r4), (r5), (r6), (func)); \
} while(0)

void syscall_sysinfo_init(void) {
    MAKE_SYSCALL_VOID(VEC_SYSINFO, FUNC_SYSINFO_INIT, 
        PARAM_NA, PARAM_NA, PARAM_NA);
}

int syscall_sysinfo_icon(uint32_t icon, uint8_t *dest) {
    MAKE_SYSCALL_INT(VEC_SYSINFO, FUNC_SYSINFO_ICON, 
        icon, dest, PARAM_NA);
}

uint64_t syscall_sysinfo_id(void) {
    uint64_t *id = NULL;

    MAKE_SYSCALL_SET(VEC_SYSINFO, FUNC_SYSINFO_ID, 
        PARAM_NA, PARAM_NA, PARAM_NA, id, uint64_t *);

    return (id != NULL) ? *id : 0;
}

int syscall_flashrom_info(uint32_t part, void *info) {
    MAKE_SYSCALL_INT(VEC_FLASHROM, FUNC_FLASHROM_INFO, 
        part, info, PARAM_NA);
}

int syscall_flashrom_read(uint32_t pos, void *dest, size_t n) {
    MAKE_SYSCALL_INT(VEC_FLASHROM, FUNC_FLASHROM_READ, 
        pos, dest, n);
}

int syscall_flashrom_write(uint32_t pos, const void *src, size_t n) {
    MAKE_SYSCALL_INT(VEC_FLASHROM, FUNC_FLASHROM_WRITE, 
        pos, src, n);
}

int syscall_flashrom_delete(uint32_t pos) {
    MAKE_SYSCALL_INT(VEC_FLASHROM, FUNC_FLASHROM_DELETE, 
        pos, PARAM_NA, PARAM_NA);
}

void syscall_gdrom_init(void) {
    MAKE_SYSCALL_VOID(VEC_MISC_GDROM, FUNC_GDROM_INIT, 
        PARAM_NA, PARAM_NA, SUPER_FUNC_GDROM);
}

void syscall_gdrom_reset(void) {
    MAKE_SYSCALL_VOID(VEC_MISC_GDROM, FUNC_GDROM_RESET, 
        PARAM_NA, PARAM_NA, SUPER_FUNC_GDROM);
}

int syscall_gdrom_check_drive(uint32_t status[2]) {
    MAKE_SYSCALL_INT(VEC_MISC_GDROM, FUNC_GDROM_DRIVE_STATUS, 
        status, PARAM_NA, SUPER_FUNC_GDROM);
}

uint32_t syscall_gdrom_send_command(uint32_t cmd, void *params) {
    uint32_t request_id = 0;

    MAKE_SYSCALL_SET(VEC_MISC_GDROM, FUNC_GDROM_SEND_COMMAND, 
        cmd, params, SUPER_FUNC_GDROM, request_id, uint32_t);
    
    return request_id;
}

int syscall_gdrom_check_command(uint32_t id, int32_t status[4]) {
    MAKE_SYSCALL_INT(VEC_MISC_GDROM, FUNC_GDROM_CHECK_COMMAND, 
        id, status, SUPER_FUNC_GDROM);
}

void syscall_gdrom_exec_server(void) {
    MAKE_SYSCALL_VOID(VEC_MISC_GDROM, FUNC_GDROM_EXEC_SERVER, 
        PARAM_NA, PARAM_NA, SUPER_FUNC_GDROM);
}

int syscall_gdrom_abort_command(uint32_t id) {
    MAKE_SYSCALL_INT(VEC_MISC_GDROM, FUNC_GDROM_ABORT_COMMAND, 
        id, PARAM_NA, SUPER_FUNC_GDROM);
}

int syscall_gdrom_sector_mode(uint32_t mode[4]) {
    MAKE_SYSCALL_INT(VEC_MISC_GDROM, FUNC_GDROM_SECTOR_MODE, 
        mode, PARAM_NA, SUPER_FUNC_GDROM);
}

void syscall_gdrom_dma_callback(uintptr_t callback, void *param) {
    MAKE_SYSCALL_VOID(VEC_MISC_GDROM, FUNC_GDROM_DMA_CALLBACK, 
        callback, param, SUPER_FUNC_GDROM);
}

int syscall_gdrom_dma_transfer(uint32_t id, const int32_t params[2]) {
    MAKE_SYSCALL_INT(VEC_MISC_GDROM, FUNC_GDROM_DMA_TRANSFER, 
        id, params, SUPER_FUNC_GDROM);
}

int syscall_gdrom_dma_check(uint32_t id, size_t *size) {
    MAKE_SYSCALL_INT(VEC_MISC_GDROM, FUNC_GDROM_DMA_CHECK, 
        id, size, SUPER_FUNC_GDROM);
}

void syscall_gdrom_pio_callback(uintptr_t callback, void *param) {
    MAKE_SYSCALL_VOID(VEC_MISC_GDROM, FUNC_GDROM_PIO_CALLBACK, 
        callback, param, SUPER_FUNC_GDROM);
}

int syscall_gdrom_pio_transfer(uint32_t id, const int32_t params[2]) {
    MAKE_SYSCALL_INT(VEC_MISC_GDROM, FUNC_GDROM_PIO_TRANSFER, 
        id, params, SUPER_FUNC_GDROM);
}

int syscall_gdrom_pio_check(uint32_t id, size_t *size) {
     MAKE_SYSCALL_INT(VEC_MISC_GDROM, FUNC_GDROM_PIO_CHECK, 
        id, size, SUPER_FUNC_GDROM);
}

int syscall_misc_init(void) {
    MAKE_SYSCALL_INT(VEC_MISC_GDROM, FUNC_MISC_INIT, 
        PARAM_NA, PARAM_NA, SUPER_FUNC_MISC);
}

int syscall_misc_setvector(uint32_t super, uintptr_t handler) {
    MAKE_SYSCALL_INT(VEC_MISC_GDROM, FUNC_MISC_SETVECTOR, 
        super, handler, SUPER_FUNC_MISC);
}

/* Function pointer type definition for system call functions. */
typedef void (*system_func)(int) __noreturn;

void syscall_system_reset(void) {
    system_func system = (system_func)(*((uintptr_t *) VEC_SYSTEM));
    system(FUNC_SYSTEM_RESET);
}

void syscall_system_bios_menu(void) {
    system_func system = (system_func)(*((uintptr_t *) VEC_SYSTEM));
    system(FUNC_SYSTEM_BIOS_MENU);
}

void syscall_system_cd_menu(void) {
    system_func system = (system_func)(*((uintptr_t *) VEC_SYSTEM));
    system(FUNC_SYSTEM_CD_MENU);
}

