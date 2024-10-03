/* KallistiOS ##version##

   browse.c
   Copyright (C) 2024 Andress Barajas

   This program demonstrates browsing and interacting with KOS's filesystem. 
   It includes features such as mounting and unmounting a FAT-formatted SD 
   card, navigating directories, displaying directory contents, and handling 
   file operations like copying and deleting files.
*/

#include <stdio.h> 
#include <stdlib.h>
#include <stdbool.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>

#include <dc/video.h>
#include <fat/fs_fat.h>
#include <kos/string.h>

#include <kos/fs_romdisk.h>
#include <kos/fs_ramdisk.h>

#include <dc/sd.h>

#include <dc/biosfont.h>

#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/maple/vmu.h>

#include "browse.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

/* Mounting FAT formatted SD card */
static kos_blockdev_t sd_dev;
static uint8_t partition_type;

int main(int argc, char **argv) {
    cont_state_t *cond;
    
    int content_count = 0;
    int selector_index = 0;
    char current_directory[BUFFER_LENGTH] = {0};
    char directory_temp[BUFFER_LENGTH] = {0};
    char directory_temp2[BUFFER_LENGTH] = {0};
    directory_file_t directory_contents[100];

    bool prompting = false;
    bool highlight_yes = true;
    bool mounted_sd = false;
    bool changed_directory = true;

    vid_set_mode(DM_640x480, PM_RGB555);

    /* Mount the SD card if one is attached to the Dreamcast */
    mounted_sd = mount_sd_fat();

    /* Start off at the root of the dreamcast and get its contents */
    strcpy(current_directory, "/");
    content_count = browse_directory(current_directory, directory_contents);
   
    unsigned int current_buttons = 0;
    unsigned int changed_buttons = 0;
    unsigned int previous_buttons = 0;

    while(true) {
        cond = get_cont_state();
        current_buttons = cond->buttons;
        changed_buttons = current_buttons ^ previous_buttons;
        previous_buttons = current_buttons;
        
        /* Enter a directory */
        if(button_pressed(current_buttons, changed_buttons, CONT_A)) {
            if(prompting) {
                prompting = false;
                changed_directory = true;

                if(strcmp(current_directory, "/sd") == 0 ||
                   strcmp(current_directory, "/ram") == 0) {
                    if(highlight_yes) {
                        memset(directory_temp, 0, BUFFER_LENGTH);
                        fs_path_append(directory_temp, current_directory, BUFFER_LENGTH);
                        fs_path_append(directory_temp, directory_contents[selector_index].filename, BUFFER_LENGTH);

                        delete_file(directory_temp, mounted_sd);
                        content_count = browse_directory(current_directory, directory_contents);
                        selector_index--;
                    }
                        
                    highlight_yes = true;
                }  
                else {
                    if(highlight_yes) {
                        memset(directory_temp, 0, BUFFER_LENGTH);
                        fs_path_append(directory_temp, current_directory, BUFFER_LENGTH);
                        fs_path_append(directory_temp, directory_contents[selector_index].filename, BUFFER_LENGTH);

                        memset(directory_temp2, 0, BUFFER_LENGTH);
                        if(mounted_sd)
                            fs_path_append(directory_temp2, "/sd", BUFFER_LENGTH);
                        else
                            fs_path_append(directory_temp2, "/ram", BUFFER_LENGTH);
                        fs_path_append(directory_temp2, basename(directory_temp), BUFFER_LENGTH);
                        fs_copy(directory_temp, directory_temp2);
                    }
                    highlight_yes = true;
                }
            }
            else {
                /* If we selected a directory */
                if(directory_contents[selector_index].is_dir) {
                    /* Navigate to it and get the directory contents */
                    memset(directory_temp, 0, BUFFER_LENGTH);
                    fs_path_append(directory_temp, current_directory, BUFFER_LENGTH);
                    fs_path_append(directory_temp, directory_contents[selector_index].filename, BUFFER_LENGTH);

                    content_count = browse_directory(directory_temp, directory_contents);
                    if(content_count > 0) {
                        realpath(directory_temp, current_directory);
                        changed_directory = true;
                        selector_index = 0;
                    } 
                }
                else if(!prompting) { /* We selected a file. Ask what we want to do with it */
                    prompting = true;
                    show_prompt(current_directory, mounted_sd, highlight_yes);
                }
            }
        }

        /* Exit a directory */
        if(button_pressed(current_buttons, changed_buttons, CONT_B)) {
            if(prompting) {
                prompting = false;
                changed_directory = true;
                highlight_yes = true;
            }
            else {
                memset(directory_temp, 0, BUFFER_LENGTH);
                /* If we are not currently in the root directory */
                if(strcmp(current_directory, "/") != 0) {
                    int copy_count = strrchr(current_directory, '/') - current_directory;
                    strncat(directory_temp, current_directory, (copy_count==0) ? 1 : copy_count );
                    /* Go the previous directory and get the directory contents */
                    content_count = browse_directory(directory_temp, directory_contents);
                    if(content_count > 0) {
                        strcpy(current_directory, directory_temp);
                        changed_directory = true;
                        selector_index = 0;
                    } 
                }
            }
        }
        
        /* Navigate the directory */
        if(button_pressed(current_buttons, changed_buttons, CONT_DPAD_DOWN)) {
            if(prompting) {
                highlight_yes = !highlight_yes;
                show_prompt(current_directory, mounted_sd, highlight_yes);
            }
            else {
                if(selector_index < (content_count-1)) {
                    ++selector_index;
                    changed_directory = true;
                }
            }
        }
        if(button_pressed(current_buttons, changed_buttons, CONT_DPAD_UP)) {
            if(prompting) {
                highlight_yes = !highlight_yes;
                show_prompt(current_directory, mounted_sd, highlight_yes);
            }
            else {
                if(selector_index > 0) {
                    --selector_index;
                    changed_directory = true;
                }
            }
        }

        /* Exit Program */
        if(button_pressed(current_buttons, changed_buttons, CONT_START))
            break;

        /* Update the screen if we navigate a directory */
        if(changed_directory) {
            changed_directory = false;
            vid_clear(0,0,0);
            draw_directory_selector(selector_index);
            draw_directory_contents(directory_contents, content_count);
        }
    }

    unmount_sd_fat();

    return 0;
}

static bool mount_sd_fat() {
    if(sd_init()) {
        fprintf(stderr, "Could not initialize the SD card. Please make sure that you "
               "have an SD card adapter plugged in and an SD card inserted.\n");
        return false;
    }

    if(sd_blockdev_for_partition(0, &sd_dev, &partition_type)) {
        fprintf(stderr, "Could not find the first partition on the SD card!\n");
        return false;
    }

    if(fs_fat_init()) {
        fprintf(stderr, "Could not initialize fs_fat!\n");
        return false;
    }

    if(fs_fat_mount("/sd", &sd_dev, FS_FAT_MOUNT_READWRITE)) {
        fprintf(stderr, "Could not mount SD card as fatfs. Please make sure the card "
            "has been properly formatted.\n");
        return false;
    }

    return true;
}

static void unmount_sd_fat() {
    fs_fat_unmount("/sd");
    fs_fat_shutdown();
    sd_shutdown();
}

static void show_prompt(char *current_directory, bool mounted_sd, bool highlight_yes) {
    if(strcmp(current_directory, "/sd") == 0 ||
       strcmp(current_directory, "/ram") == 0)  {
            if(mounted_sd)
                prompt_message("Delete this file from SD card?", highlight_yes);
            else
                prompt_message("Delete this file from /ram directory?", highlight_yes);
    }
    else {
        if(mounted_sd)
            prompt_message("Copy this file to SD card?", highlight_yes);
        else 
            prompt_message("Copy this file to /ram directory?", highlight_yes);
    }
}

static void delete_file(char *filename, bool mounted_sd) {
    void *filedata = NULL;
    unsigned int filesize = 0;
    
    if(mounted_sd)
        remove(filename);
    else {
        fs_ramdisk_detach(basename(filename), &filedata, &filesize);
        free(filedata);
    }
}

static int browse_directory(char *directory, directory_file_t *directory_contents) {
    int count = 0;
    DIR *d;
    struct dirent *entry;

    /* Open the directory */
    if (!(d = opendir(directory))) {
        fprintf(stderr, "browse_directory: opendir failed for %s\n", directory);
        return 0;
    }

    /* Clear out all files */
    memset(directory_contents, 0, sizeof(directory_contents[0])*100);

    /* Read all the filenames in the directory */
    while((entry = readdir(d)) && count < 100) {
        directory_contents[count].is_dir = (entry->d_type == DT_DIR);
        strncpy(directory_contents[count].filename, entry->d_name, sizeof(directory_contents[count].filename) - 1);
        directory_contents[count].filename[sizeof(directory_contents[count].filename) - 1] = '\0';  // Ensure null-termination
        count++;
    }

    /* Close directory */
    closedir(d);

    return count;
}

static void prompt_message(char *message, bool highlight_yes) {
    int x = 20 + BFONT_HEIGHT, y = 350;
    int color = 1;

    bfont_set_foreground_color(0xFFFFFFFF);
    bfont_set_background_color(0x00000000);
    bfont_draw_str(vram_s + y*SCREEN_WIDTH+x, SCREEN_WIDTH, color, message);

    if(highlight_yes) {
        bfont_set_foreground_color(0x00000000);
        bfont_set_background_color(0xFFFFFFFF);
    }
    else {
        bfont_set_foreground_color(0xFFFFFFFF);
        bfont_set_background_color(0x00000000);
    }
    y += BFONT_HEIGHT;
    bfont_draw_str(vram_s + y*SCREEN_WIDTH+x, SCREEN_WIDTH, color, "YES");

    if(highlight_yes) {
        bfont_set_foreground_color(0xFFFFFFFF);
        bfont_set_background_color(0x00000000);
    }
    else {
        bfont_set_foreground_color(0x00000000);
        bfont_set_background_color(0xFFFFFFFF);
    }
    y += BFONT_HEIGHT;
    bfont_draw_str(vram_s + y*SCREEN_WIDTH+x, SCREEN_WIDTH, color, "NO");
}

static void draw_directory_selector(int index) {
    int x = BFONT_HEIGHT, y = BFONT_HEIGHT + (index * BFONT_HEIGHT);
    int color = 1;

    bfont_set_foreground_color(0xFFFFFFFF);
    bfont_set_background_color(0x00000000);
    bfont_draw_str(vram_s + y*SCREEN_WIDTH+x, SCREEN_WIDTH, color, ">");
}

static void draw_directory_contents(directory_file_t *directory_contents, int num) {
    int x = 20 + BFONT_HEIGHT, y = BFONT_HEIGHT;
    int color = 1;
    int count = 0;
    char str[80];

    bfont_set_foreground_color(0xFFFFFFFF);
    bfont_set_background_color(0x00000000);
    for(int i=0;i<num;i++) {
        if(directory_contents[i].is_dir) {
            snprintf(str, sizeof(str), "%-40s%s", directory_contents[i].filename, "< DIR >");
            bfont_draw_str(vram_s + y*SCREEN_WIDTH+x, SCREEN_WIDTH, color, str);
        }
        else {
            bfont_draw_str(vram_s + y*SCREEN_WIDTH+x, SCREEN_WIDTH, color, directory_contents[i].filename);
        }
        count++;
        y += BFONT_HEIGHT;

        if(y >= (SCREEN_HEIGHT - BFONT_HEIGHT))
            break;
    }
}

static cont_state_t *get_cont_state() {
    maple_device_t *cont;
    cont_state_t *state;

    cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
    if(cont) {
        state = (cont_state_t*)maple_dev_status(cont);
        return state;
    }

    return NULL;
}

static bool button_pressed(unsigned int current_buttons, unsigned int changed_buttons, unsigned int button) {
    return !!(changed_buttons & current_buttons & button);
}
