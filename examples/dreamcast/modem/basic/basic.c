/* KallistiOS ##version##

   basic.c
   Copyright (C)2004 Nick Kochakian

   Connecting without events
*/

#include <stdio.h>
#include <kos.h>
#include "dc/modem/modem.h"

#define DATA_BUFFER_LENGTH 128

int buttonPressed(int button) {
    maple_device_t *cont;
    cont_state_t *state;

    cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

    if(cont) {
        state = (cont_state_t *)maple_dev_status(cont);

        if(state) {
            return state->buttons & button;
        }
    }

    return 0;
}

int main(int argc, const char* argv[]) {
    int           answerMode = 0;
    unsigned char data[DATA_BUFFER_LENGTH];
    int           i;
    int           byteCount;

    if(!modem_init()) {
        printf("modem_init failed!\n");
        return 1;
    }

    printf("\nDreamcast modem - example 1\n");
    printf("Press START to exit\n\n");

    if(answerMode)
        printf("Answer mode - Call the Dreamcast. It will pick up after one ring.\n\n");
    else
        printf("Remote mode - Connect the phone line to your computer's modem and use ATA to\n              \"answer\" the Dreamcast.\n\n");

    printf("Once the modems are connected you can send data to the Dreamcast,\n");
    printf("and the Dreamcast can send data to the remote modem by pressing the\n");
    printf("A button.\n\n");

    while(!buttonPressed(CONT_START)) {
        modem_set_mode(answerMode ? MODEM_MODE_ANSWER :
                       MODEM_MODE_REMOTE,
                       MODEM_SPEED_V8_AUTO);

        printf("Waiting for a connection..\n");

        while(modem_is_connecting()) {
            if(buttonPressed(CONT_START)) {
                printf("Connection aborted\n");
                return 0;
            }
        }

        if(modem_is_connected()) {
            printf("Connected at %dbps\n", (int)modem_get_connection_rate());

            while(!buttonPressed(CONT_START) && modem_is_connected()) {
                /* Receive data */
                if(modem_has_data()) {
                    byteCount = modem_read_data(data, DATA_BUFFER_LENGTH);

                    for(i = 0; i < byteCount; i++)
                        printf("%c", (data[i] >= 32 && data[i] <= 126) ?
                               data[i] : ' ');
                }

                /* Send data */
                if(buttonPressed(CONT_A)) {
                    for(i = 0; i < 8; i++)
                        data[i] = (int)'a' + i;

                    modem_write_data(data, 8);
                }
            }

            if(modem_is_connected()) {
                printf("Disconnecting..\n");
                modem_disconnect();

                while(buttonPressed(CONT_START));
            }
            else
                printf("Disconnected\n");
        }
        else
            printf("Connection failed\n");
    }

    return 0;
}
