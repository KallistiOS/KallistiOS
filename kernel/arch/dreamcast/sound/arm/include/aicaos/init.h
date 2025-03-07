#ifndef __ARM_AICA_INIT_H
#define __ARM_AICA_INIT_H

#define aicaos_initcall(_fn) \
__attribute__((section(".init_table"))) void *_fn ## _initcall = (void *)(_fn);

#endif /* __ARM_AICA_INIT_H */
