/* LibADX for KallistiOS ##version##

   libADXPlay.c
   Simple "Hello World" style example of ADX playback.

   Copyright (C) 2012 Josh 'PH3NOM' Pearson <ph3nom.dcmc@gmail.com>
   Copyright (C) 2024 The KOS Team and contributors
*/

#include <stdio.h>

#include <kos/dbgio.h>

#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/sound/stream.h>

#include <kos/init.h>
#include <kos.h>

#include <adx/adx.h> /* ADX Decoder Library */
#include <adx/snddrv.h> /* Direct Access to Sound Driver */

#define CONT_RESUME  0x01
#define CONT_PAUSE   0x02
#define CONT_RESTART 0x03
#define CONT_STOP    0x04
#define CONT_VOLUP   0x05
#define CONT_VOLDN   0x06

int check_cont()
{
    int ret=0;
    maple_device_t *cont;
	cont_state_t *state;		
    cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

	if(cont)
	{
		state = (cont_state_t *)maple_dev_status(cont);
		if (!state)
			ret = 0;
		if (state->buttons & CONT_START)
            ret = CONT_STOP;               
		if (state->buttons & CONT_X)
            ret = CONT_RESTART;
		if (state->buttons & CONT_A) 
            ret = CONT_PAUSE;
		if (state->buttons & CONT_B) 
            ret = CONT_RESUME;
		if (state->buttons & CONT_DPAD_UP) 
            ret = CONT_VOLUP;
		if (state->buttons & CONT_DPAD_DOWN) 
            ret = CONT_VOLDN;   
	}
	return ret;
}

int main()
{
    /* Print some text to the screen */
	int o = 20*640+20;
	bfont_set_encoding(BFONT_CODE_ISO8859_1);
    bfont_draw_str(vram_s+o,640,1,"LibADX (C) PH3NOM 2012"); o+=640*48;
    printf("LibADX (C) PH3NOM 2012\n");
    
    /* Start the ADX stream, with looping enabled */
    if( adx_dec( "/rd/test.adx", 1 ) < 1 )
    {
        printf("Invalid ADX file\n");
        return 0;
    }

    /* Wait for the stream to start */
    while( snddrv.drv_status == SNDDRV_STATUS_NULL )
        thd_pass(); 
    
    bfont_draw_str(vram_s+o,640,1,"Press Start to stop, press X to restart");
    o+=640*48;
    bfont_draw_str(vram_s+o,640,1,"Press A to pause, press B to resume");
    o+=640*48;
    bfont_draw_str(vram_s+o,640,1,"Press UP or Down to increase/decrease volume");
             
    /* Check for user input and eof */
	while( snddrv.drv_status != SNDDRV_STATUS_NULL ) {               
       int vol;
       switch (check_cont()) {
            case CONT_RESTART:
                 if(adx_restart())
                    printf("ADX streaming restarted\n");
                 break;
            case CONT_STOP: 
                 if(adx_stop())
                    printf("ADX streaming stopped\n");
                 break;
            case CONT_PAUSE: 
                 if(adx_pause())
                    printf("ADX streaming paused\n");
                 break;
            case CONT_RESUME: 
                 if(adx_resume())
                    printf("ADX streaming resumed\n");
                 break;
            case CONT_VOLUP: 
                 vol = snddrv_volume_up();
                 printf("SNDDRV: Volume set to %i%s\n", ((vol*100)/255), "%");
                 break;
            case CONT_VOLDN: 
                 vol = snddrv_volume_down();
                 printf("SNDDRV: Volume set to %i%s\n", ((vol*100)/255), "%");
                 break;
            default:
                 break; 
       }
       thd_sleep(50);
    } /* when (snddrv.drv_status == SNDDRV_STATUS_NULL) the stream is finished*/

    printf( "LibADX Example Finished\n");
    
    return 0;
}
