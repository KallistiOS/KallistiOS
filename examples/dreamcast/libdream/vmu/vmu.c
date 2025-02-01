/* Note: this is just a low-level example; in theory you can use
   the fs_vmu functions to do file-level VMU access. Of course
   that is probably not working at the moment.. ^_^; */

/* Thanks to Marcus Comstedt for this VMU format info. */

#include <stdio.h>
#include <string.h>

#include <dc/biosfont.h>
#include <dc/video.h>

#include <dc/maple/vmu.h>


/* Draws one file entry, along with its "description" in the
   boot rom file manager. */
static int off1 = 20 + 36;

void draw_one(maple_device_t *addr, char *fn, uint16 hdrblock) {
    bfont_draw_str(vram_s + off1 * 640 + 10, 640, 0, "File ");
    bfont_draw_str(vram_s + off1 * 640 + 10 + 5 * 12, 640, 0, fn);

    if(hdrblock) {
        uint8 buf[1024];

        if(vmu_block_read(addr, hdrblock, buf)) {
            printf("Can't read file header at %d\n", hdrblock);
            return;
        }

        buf[0x10 + 32] = 0;
        bfont_draw_str(vram_s + off1 * 640 + 10 + (6 + strlen(fn)) * 12, 640, 0,
                       (char *)(buf + 0x10));
    }

    off1 += 24;
}

/* We only do the monochrome one here to avoid having to
   parse through the FAT. */
void draw_icondata(maple_device_t *addr, uint16 block) {
    uint8 buf[512], *icon;
    uint16 *buf16 = (uint16*)buf, *vr = vram_s + 20 * 640 + 20;
    uint16 monoicon;
    int x, y;

    if(vmu_block_read(addr, block, buf)) {
        printf("Can't read ICONDATA block %d\n", block);
        return;
    }

    monoicon = buf16[0x10 / 2];

    if(monoicon > (512 - 128)) {
        printf("Monochrome icon is too far in\n");
        return;
    }

    icon = buf + monoicon;

    for(y = 0; y < 32; y++)
        for(x = 0; x < 32; x++) {
            if(icon[(y * 32 + x) / 8] & (0x80 >> (x & 7)))
                vr[y * 640 + x] = 0;
            else
                vr[y * 640 + x] = 0xffff;
        }
}

/* The full read test */
void vmu_read_test(void) {
    int i, n, drawn = 0;
    maple_device_t *addr = maple_enum_type(0, MAPLE_FUNC_MEMCARD);
    uint8 *ent;
    uint8 buf[512];

    uint16 *buf16 = (uint16*)buf, dirblock, dirlength, *ent16;

    uint16 icondata = 0;

    /* Make sure we have a VMU */
    if(!addr) {
        printf("No VMU present!\n");
        return;
    }

    /* Read the root block and find out where the directory is located */
    if(vmu_block_read(addr, 255, buf)) {
        printf("Can't read VMU root block\n");
    }

    dirblock = buf16[0x4a / 2];
    dirlength = buf16[0x4c / 2];

    /* Draw entries and look for the ICONDATA.VMS entry */
    for(n = dirlength; n > 0; n--) {
        /* Read one dir block */
        if(vmu_block_read(addr, dirblock, buf) != 0) {
            printf("Can't read VMU block %d\n", dirblock);
            return;
        }

        /* Each block has 16 entries */
        for(i = 0; i < 16; i++) {
            ent = buf + i * 32;

            if(!*ent) continue;

            /* Check the filename; for normal files, draw an
               entry on the screen. If it's ICONDATA_VMS then
               just keep track of the block. */
            ent16 = (uint16*)ent;
            ent[4 + 12] = 0;

            if(!strcmp((char *)(ent + 4), "ICONDATA_VMS"))
                icondata = ent16[1];
            else {
                /* We don't handle FAT traversal... */
                if(ent16[0x1a / 2]) {
                    draw_one(0, (char*)(ent + 4), 0);
                }
                else {
                    draw_one(addr, (char*)(ent + 4), ent16[1]);
                }

                /* More than 16 won't fit. */
                if((drawn++) >= 16) {
                    dirblock = 0;
                    i = 16;
                    break;
                }
            }
        }

        dirblock--;
    }

    /* Did we find the ICONDATA_VMS file? If so, draw it.. */
    if(icondata)
        draw_icondata(addr, icondata);
}

int main(int argc, char **argv) {
    /* Do VMU read test */
    vmu_read_test();

    thd_sleep(5 * 1000);

    return 0;
}
