/* KallistiOS ##version##

   example.c
   Copyright (C) 2023 Aaron Glazer

   This example demonstrates basic usage of KOS's port
   of micropython via its C API.
*/

#include <stdio.h>
#include <stdlib.h>

#include <kos.h>

#include <micropython/py/parse.h>
#include <micropython/py/lexer.h>
#include <micropython/py/gc.h>
#include <micropython/py/nlr.h>
#include <micropython/py/compile.h>
#include <micropython/py/runtime.h>
#include <micropython/py/stackctrl.h>
#include <micropython/py/qstr.h>
#include <micropython/py/obj.h>

static char mp_heap[8 * 1024];

static void load_module(void) {
    mp_lexer_t *lex = mp_lexer_new_from_file(qstr_from_str("/rd/script.py"));
    mp_parse_tree_t parse_tree = mp_parse(lex, MP_PARSE_FILE_INPUT);
    mp_obj_t mod = mp_compile(&parse_tree, lex->source_name, false);
    mp_call_function_0(mod);
}

static void demo(void) {
    printf("(entering script)\n");
    load_module();
    printf("(exited script)\n");

    mp_obj_t fn;
    mp_obj_t res;
    mp_obj_t five = mp_obj_new_int(5);

    fn = mp_load_name(qstr_from_str("f"));
    res = mp_call_function_1(fn, five);
    printf("f(5): ");
    mp_obj_print(res, PRINT_REPR);
    printf("\n");

    fn = mp_load_name(qstr_from_str("g"));
    res = mp_call_function_1(fn, five);
    printf("g(5): ");
    mp_obj_print(res, PRINT_REPR);
    printf("\n");

    mp_obj_t sum_obj = mp_load_name(qstr_from_str("sum"));
    mp_int_t sum = mp_obj_int_get_checked(sum_obj);
    printf("sum: %d\n", sum);

    printf("globals:\n");
    mp_obj_dict_t* globals = mp_globals_get();
    mp_map_t* map = &globals->map;
    for (size_t i = 0; i < map->alloc; i++) {
        printf("  ");
        if (map->table[i].key != MP_OBJ_NULL) {
            mp_obj_print(map->table[i].key, PRINT_REPR);
        } else {
            printf("(nil)");
        }
        printf(": %p\n", map->table[i].value);
    }
}

int main(int argc, const char* argv[]) {
    mp_stack_ctrl_init();
    gc_init(mp_heap, mp_heap + sizeof(mp_heap));
    mp_init();

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        demo();
        nlr_pop();
    } else {
        fprintf(stderr, "demo ran into an uncaught exception!\n");
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
        exit(EXIT_FAILURE);
    }

    mp_deinit();

    return EXIT_SUCCESS;
}
