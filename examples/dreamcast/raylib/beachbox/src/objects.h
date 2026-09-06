/* KallistiOS ##version##
   examples/dreamcast/raylib/beachbox/src/objects.h
   Copyright (C) 2024 Agustín Bellagamba
   Copyright (C) 2024 Cypress
*/

#ifndef BBOX_OBJECTS_H
#define BBOX_OBJECTS_H

// Initializes the objects
void init_objects(void);

// Updates the objects
void update_objects(void);

// Draws the objects
void draw_objects(void);

// Returns the current object speed
float get_current_object_speed(void);

// Resets the object speed back to the initial value
void reset_object_speed(void);

#endif