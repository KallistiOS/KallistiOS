#include <stdio.h>

#include <unistd.h>
#include <sys/ioctl.h>

#include <dc/cdrom.h>
#include <arch/cache.h>

#define AICA_REG(n) (*(volatile uint32_t*)(0xA0700000+n))
#define GD_PROTECTION  (*(volatile uint32_t*)0xA05F74B8)
#define SYSCALLS_32(n) (*(volatile uint32_t*)(0xAC000000 + n))

int main(int argc, char *argv[]) {

   // Patch system calls to unprotect the GD-ROM DMA for the entire address space
   SYSCALLS_32(0x1C20) = 0x8843007F;
   SYSCALLS_32(0x23FC) = 0x8843007F;

   // Flush the instruction cache for the patch
   icache_flush_range(0xC000000, 0xC010000);

   // Get the file sector and size
   int fd = open("/cd/test.pcm", O_RDONLY);
   int file_sector = ioctl(fd, IOCTL_ISO9660_GET_SECTOR);
   off_t file_size = lseek(fd, 0, SEEK_END);
   close(fd);

   printf("/cd/test.pcm start sector %d, size %ld\n", file_sector, file_size);


   // DMA directly to sound ram
   int rv = cdrom_read_sectors_ex((void*)0x00800000, 150 + file_sector, file_size/2048, CDROM_READ_DMA);
   if (rv) {
      printf("cdrom_read_sectors_ex failed, %d\n", rv);
      return 1;
   } else {
      printf("cdrom_read_sectors_ex succeeded\n");
   }

   // Play the DMA'd data in a loop
   AICA_REG(0x00) = (1 << 9); // KEY_ON, CA_low
   AICA_REG(0x04) = 0; // CA_high
   AICA_REG(0x08) = 0; // LSA
   AICA_REG(0x0C) = 65534; // LEA
   AICA_REG(0x10) = 0x1F; // D2R D1R AR
   AICA_REG(0x14) = 0; // LS KRS DL RR
   AICA_REG(0x18) = 0; // OCT FNS
   AICA_REG(0x1C) = 0; // RE LFOF PFLOWS PFLOS ALFOWS ALFOS
   AICA_REG(0x20) = 0; // IMXL ISEL
   AICA_REG(0x24) = 0xf << 8; // DISDL DIPAN
   AICA_REG(0x28) = 0x4; // TL Q
   AICA_REG(0x2C) = 0x1FF8; // FLV0
   AICA_REG(0x30) = 0x1FF8; // FLV1
   AICA_REG(0x34) = 0x1FF8; // FLV2
   AICA_REG(0x38) = 0x1FF8; // FLV3
   AICA_REG(0x3C) = 0x1FF8; // FLV4
   AICA_REG(0x40) = 0; // FAR FD1R
   AICA_REG(0x44) = 0; // FD2R FRR


   AICA_REG(0x00) |= 0xC000; // KEY_ON, EX

   // wait for ever
   for(;;);

   return 0;
}
