/* KallistiOS ##version##

   signal.c
   Copyright (C) 2024 Falco Girgis
*/

#define _GNU_SOURCE 

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <fenv.h>
#include <kos.h>

static struct {
   jmp_buf jump_buff;
   volatile sig_atomic_t signal;
   volatile sig_atomic_t fired;
} sig_data = { 0 };

static void sig_handler(int signum) {
   char buffer[SIG2STR_MAX];

   if(sig2str(signum, buffer) < 0) {
      strcpy(buffer, "UNKNOWN");
   }

   printf("CAUGHT: %d [%s]!\n", signum, buffer);

   sig_data.signal = signum;
   sig_data.fired = true;

   printf("\tReturning from handler... ");
   fflush(stdout);

   longjmp(sig_data.jump_buff, 1);
}

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_SIGNALS);

static bool sig_tester(int signum, void (*activator)(void)) {
   char buffer[SIG2STR_MAX];
   bool success = true;

   if(sig2str(signum, buffer) < 0) {
      fprintf(stderr, "Failed to retrieve name for signal: %d\n", signum);
      strcpy(buffer, "UNKNOWN");
      success = false;
   }

   printf("\nTesting Signal: %d [%s]\n", signum, buffer);

   sig_data.signal = 0;
   sig_data.fired = false;

   printf("\tInstalling handler... ");
   signal(signum, sig_handler);
   printf("DONE.\n");

   if(!setjmp(sig_data.jump_buff)) {
      printf("\tRaising signal... ");
      fflush(stdout);
      activator();
   } else {
      printf("BACK.\n");
   }

   printf("\tValidating... ");
   if(sig_data.fired) {
      if(sig_data.signal == signum)
         printf("SUCCESS!\n");
      else {
         fprintf(stderr, "FAILURE!\n");
         fprintf(stderr, "\t\tExpected: %d, Captured: %d\n", 
                 signum, sig_data.signal);
         success = false;
      }
   } else {
      fprintf(stderr, "FAILURE!\n");
      fprintf(stderr, "\t\tSignal not detected!\n");
      success = false;
   }

   return success;
}

static void div_zero(void) {
   feclearexcept(FE_ALL_ACCEPT);
   feenableexcept(FE_DIVBYZERO);

   //fenv_t env;
   //fegetenv(&env);
   feraiseexcept(FE_DIVBYZERO);

   if(__builtin_sh_get_fpscr() & 0xffb)
   printf("WORKX\n");


#if 0
   volatile float a = 100.0f;
   volatile float b = 0.0f;
   volatile float c = a / b;
   (void)c;
#endif


}

static void null_deref(void) {
   *(volatile int *)(0) = 3;
}

int main(int argc, const char* argv[]) {
   bool success = true;

   printf("Welcome to the C standard signal tester!\n");

   success &= sig_tester(SIGABRT, abort);
   success &= sig_tester(SIGFPE,  div_zero);
   success &= sig_tester(SIGSEGV, null_deref);

   if(success) {
      printf("\nInterpreting Results... SUCCESS!\n");
      return EXIT_SUCCESS;
   } else {
      fprintf(stderr, "\nInterpreting Results... FAILURE!\n");
      return EXIT_FAILURE;
   }
}