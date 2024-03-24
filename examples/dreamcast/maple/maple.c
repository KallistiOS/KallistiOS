/*  KallistiOS ##version##

    maple.c
    Copyright (C) 2024 Falco Girgis
*/

/*
    This example demonstrates how to detect devices getting attached to and
    removed from the maple bus (controller ports) along with querying for and
    displaying the device details upon detection.

    Simply connect or disconnect maple devices or press START on a controller
    to exit.

    NOTE: All program output is in the terminal; you will not see anything
          drawn to the screen!
*/

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include <dc/maple.h>
#include <dc/maple/controller.h>

/* Called every time a new maple device is detected. */
static void on_maple_attach(maple_device_t *dev, void *user_data) {
    unsigned *attach_counter = (unsigned *)user_data;

    printf("Maple device attached [%c%d]: %s\n",
           'A' + dev->port, dev->unit, dev->info.product_name);

    printf("\t%-15s: %55s\n", "Functions", maple_pcaps(dev->info.functions));
    printf("\t%-15s: %55s\n", "Driver", dev->drv->name);
    printf("\t%-15s: %55d\n", "Region Code", dev->info.area_code);
    printf("\t%-15s: %55d\n", "Orientation", dev->info.connector_direction);
    printf("\t%-15s: %55d\n", "Standby Power", dev->info.standby_power);
    printf("\t%-15s: %55d\n", "Max Power", dev->info.max_power);
    printf("\t%-15s: %55s\n", "License", dev->info.product_license);

    ++(*attach_counter);
}

/* Called every time an existing maple device is removed. */
static void on_maple_detach(maple_device_t *dev, void *user_data) {
    unsigned *detach_counter = (unsigned *)user_data;

    printf("Maple device detached [%c%d]: %s\n",
           'A' + dev->port, dev->unit, dev->info.product_name);

    ++(*detach_counter);
}

/* Flag for whether to exit the example. */
static volatile bool quit = false;

/* Called whenever start is pressed on any controller. */
static void on_press_start(uint8_t address, uint32_t buttons) {
    (void)address;
    (void)buttons;

    quit = true;
}

/* Program entry point */
int main(int argc, char* argv[]) {
    /* Event counters. */
    unsigned attach_counter = 0, detach_counter = 0;

    /* Subscribe a callback to maple attach events. */
    maple_attach_callback(MAPLE_FUNC_ALL, on_maple_attach, &attach_counter);
    /* Subscribe a callback to maple detach events. */
    maple_detach_callback(MAPLE_FUNC_ALL, on_maple_detach, &detach_counter);

    /* Subscribe a callback to be notified whenever the start button is pressed
       on any connected controller. */
    cont_btn_callback(0, CONT_START, on_press_start);

    printf("Listening for device hotplug events...\n");;

    /* Do nothing while we wait for maple events */
    while(!quit);

    printf("Attached Events: %u, Detached Events: %u\n",
           attach_counter, detach_counter);

    return EXIT_SUCCESS;
}