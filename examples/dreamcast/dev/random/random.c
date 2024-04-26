/* KallistiOS ##version##

   random.c
   Copyright (C) 2023 Luke Benstead
*/

#include <kos.h>
#include <stdint.h>

KOS_INIT_FLAGS(INIT_DEFAULT);

int main(int argc, char **argv) {
   const int buffer_size = 256;
   uint8_t buffer[buffer_size];

   FILE* urandom = fopen("/dev/urandom", "r");
   if(!urandom) {
      fprintf(stderr, "Unable to open /dev/urandom for reading\n");
      return 1;
   }

   size_t bytes = fread(buffer, 1, buffer_size, urandom);
   fclose(urandom);

   if(bytes != buffer_size) {
      fprintf(stderr, "Failed to read the correct number of bytes\n");
      return 2;
   }

   printf("Generated the following random bytes:\n\n");
   for(int i = 0; i < buffer_size; ++i) {
      printf("0x%02X ", (int) buffer[i]);
   }
   printf("\n\n");

   return 0;
}
