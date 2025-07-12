/*
   AICAOS

   init.h
   Copyright (C) 2025 Paul Cercueil

   Core initialization API
*/
#ifndef __AICAOS_INIT_H
#define __AICAOS_INIT_H

#define aicaos_initcall(_fn) \
__attribute__((section(".init_table"))) void *_fn ## _initcall = (void *)(_fn)

#endif /* __AICAOS_INIT_H */
