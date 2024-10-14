/* KallistiOS ##version##

   library-dependence.h
   Copyright (C) 2024 Ruslan Rostovtsev

   This example program simply show how library works.
*/

#include <stdint.h>

/**
 * @brief Exported test functions
 */
int library_test_func(int arg);
void library_test_func2(const char *arg);

typedef int (*library_test_func_t)(int arg);
typedef void (*library_test_func2_t)(const char *arg);
