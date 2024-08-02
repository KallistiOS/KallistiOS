/* KallistiOS ##version##

   img.c
   (c)2002 Megan Potter

   Platform independent image handling
*/

#include <assert.h>
#include <stdlib.h>
#include <kos/img.h>

/* Free a kos_img_t which was created by an image loader; set struct_also to non-zero
   if you want it to free the struct itself as well. */
void kos_img_free(kos_img_t *img, int struct_also) {
    assert(img != NULL);

    /* Free the image data (if any) */
    if(img->data && !(KOS_IMG_FMT_I(img->fmt) & KOS_IMG_NOT_OWNER))
        free(img->data);

    /* Free the struct itself */
    if(struct_also)
        free(img);
}


