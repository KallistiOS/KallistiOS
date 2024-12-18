
#ifndef __BROWSE_H_
#define __BROWSE_H_

#define BUFFER_LENGTH 512

typedef struct directory_file {
    char filename[256];
    bool  is_dir;
} directory_file_t;

static bool mount_sd_fat();
static void unmount_sd_fat();

static void show_prompt(char *current_directory, bool mounted_sd, bool highlight_yes);
static void delete_file(char *filename, bool mounted_sd);

static void prompt_message(char *message, bool highlight_yes);

static void draw_directory_selector(int index);
static void draw_directory_contents(directory_file_t *directory_contents, int num);

static int browse_directory(char *directory, directory_file_t *directory_contents);

static cont_state_t *get_cont_state();
static bool button_pressed(unsigned int current_buttons, unsigned int changed_buttons, unsigned int button);

#endif
