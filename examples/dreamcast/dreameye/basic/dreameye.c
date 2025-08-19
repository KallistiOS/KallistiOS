/* KallistiOS ##version##

   dreameye.c
   Copyright (C) 2009 Lawrence Sebald
*/

#include <stdio.h>
#include <stdlib.h>

#include <dc/maple.h>
#include <dc/maple/dreameye.h>

int main(int argc, char *argv[]) {
    maple_device_t *dreameye;
    dreameye_state_t *state;
    uint8_t *buf;
    int size, err;
    FILE *fp;

    printf("KallistiOS Dreameye Test program\n");
    printf("Attempting to find a connected Dreameye device...\n");

    dreameye = maple_enum_type(0, MAPLE_FUNC_CAMERA);

    if(!dreameye) {
        printf("Couldn't find any attached devices, bailing out.\n");
        return 0;
    }

    state = (dreameye_state_t *)maple_dev_status(dreameye);

    printf("Attempting to grab the number of saved images...\n");
    dreameye_get_image_count(dreameye, 1);

    printf("Image Count is %s -- (%d)\n",
           state->image_count_valid ? "valid" : "invalid", state->image_count);

    printf("Attempting to grab the first image.\n");
    err = dreameye_get_image(dreameye, 2, &buf, &size);

    if(err != MAPLE_EOK) {
        printf("Error was: %d\n", err);
        return 0;
    }

    printf("Image received successfully, size %d bytes\n", size);

    fp = fopen("/pc/image.jpg", "wb");

    if(!fp) {
        printf("Could not open /pc/image.jpg for writing\n");
        free(buf);
        return 0;
    }

    fwrite(buf, size, 1, fp);
    fclose(fp);
    free(buf);

    printf("That's all for now.\n");
    return 0;
}
