/* KallistiOS ##version##

   lftpd.c
   Copyright (C) 2018 Jason von Nieda
   Copyright (C) 2026 BruceLeet
   Copyright (C) 2026 Paul Cercueil
   Copyright (C) 2026 Andy Barajas
*/

/*
   A small FTP server example for the Dreamcast, ported from lftpd. It brings up
   KOS networking, mounts any attached storage (IDE / SD card), and serves the
   VFS root over FTP so files can be moved to and from the Dreamcast with a
   standard FTP client during development. Press START on the controller to
   exit.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

#include <kos.h>
#include <dc/video.h>
#include <dc/biosfont.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/g1ata.h>
#include <dc/sd.h>
#include <fat/fs_fat.h>
#include <arch/timer.h>

#include "lftpd.h"

#include "private/lftpd_status.h"
#include "private/lftpd_inet.h"
#include "private/lftpd_log.h"
#include "private/lftpd_string.h"
#include "private/lftpd_io.h"

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_NET);

#define FTP_PORT      21
#define DHCP_WAIT_MS  5000
#define PASV_PORT_BASE 10000

static volatile uint16_t g_pasv_port = PASV_PORT_BASE;

/* Block devices for the mounted storage. fs_fat_mount() keeps the pointer to
   these (it does not copy them), so they must live for the whole program --
   hence file scope, not stack locals. */
static kos_blockdev_t ide_bd;
static kos_blockdev_t sd_bd;

// https://tools.ietf.org/html/rfc959
// https://tools.ietf.org/html/rfc2389#section-2.2
// https://tools.ietf.org/html/rfc3659
// https://tools.ietf.org/html/rfc5797
// https://tools.ietf.org/html/rfc2428#section-3 EPSV
// https://en.wikipedia.org/wiki/List_of_FTP_commands

typedef struct {
    char *command;
    int (*handler) (lftpd_client_t *client, const char *arg);
} command_t;

static command_t commands[];

static int send_response(int socket, int code, bool include_code,
                         bool multiline_start, const char *format, ...) {
    va_list args;
    va_start(args, format);
    char message[256];
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    char response[512];
    if(include_code) {
        if(multiline_start) {
            snprintf(response, sizeof(response), "%d-%s%s", code, message, CRLF);
        }
        else {
            snprintf(response, sizeof(response), "%d %s%s", code, message, CRLF);
        }
    }
    else {
        snprintf(response, sizeof(response), "%s%s", message, CRLF);
    }

    return lftpd_inet_write_string(socket, response);
}

#define send_simple_response(socket, code, format, ...) send_response(socket, code, true, false, format, ##__VA_ARGS__)

#define send_multiline_response_begin(socket, code, format, ...) send_response(socket, code, true, true, format, ##__VA_ARGS__)

#define send_multiline_response_line(socket, format, ...) send_response(socket, 0, false, false, format, ##__VA_ARGS__)

#define send_multiline_response_end(socket, code, format, ...) send_response(socket, code, true, false, format, ##__VA_ARGS__)

static int send_list(int socket, const char *path) {
    // https://files.stairways.com/other/ftp-list-specs-info.txt
    // http://cr.yp.to/ftp/list/binls.html
    static const char *directory_format = "drwxr-xr-x    1 ftp      ftp      %8ld Jan 01  2000 %s";
    static const char *file_format = "-rw-r--r--    1 ftp      ftp      %8ld Jan 01  2000 %s";

    DIR *dp = opendir(path);
    if(dp == NULL) {
        return -1;
    }

    struct dirent *entry;
    while((entry = readdir(dp))) {
        /* Skip . and .. */
        if(entry->d_name[0] == '.' &&
            (entry->d_name[1] == '\0' ||
             (entry->d_name[1] == '.' && entry->d_name[2] == '\0')))
            continue;
        if(entry->d_name[0] == '\0')
            continue;

        char *file_path = lftpd_io_canonicalize_path(path, entry->d_name);
        struct stat st;
        if(stat(file_path, &st) == 0) {
            long size = (long)st.st_size;
            if(size < 0) size = 0;
            if(S_ISDIR(st.st_mode)) {
                send_multiline_response_line(socket, directory_format, size, entry->d_name);
            }
            else {
                send_multiline_response_line(socket, file_format, size, entry->d_name);
            }
        }
        free(file_path);
    }

    closedir(dp);

    return 0;
}

static int send_nlst(int socket, const char *path) {
    DIR *dp = opendir(path);
    if(dp == NULL) {
        return -1;
    }

    struct dirent *entry;
    while((entry = readdir(dp))) {
        char *file_path = lftpd_io_canonicalize_path(path, entry->d_name);
        struct stat st;
        if(stat(file_path, &st) == 0) {
            if(S_ISREG(st.st_mode)) {
                send_multiline_response_line(socket, entry->d_name);
            }
        }
        free(file_path);
    }

    closedir(dp);

    return 0;
}

static int send_file(int socket, const char *path) {
    FILE *file = fopen(path, "rb");
    if(file == NULL) {
        lftpd_log_error("fopen failed: %s", path);
        return -1;
    }

    /* Get file size for progress */
    fseek(file, 0, SEEK_END);
    long total = ftell(file);
    fseek(file, 0, SEEK_SET);
    lftpd_log_info("RETR %s (%ld bytes)", path, total);

    unsigned char buffer[4096];
    int read_len;
    long sent = 0;
    while((read_len = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        unsigned char *p = buffer;
        while(read_len) {
            int write_len = write(socket, p, read_len);
            if(write_len < 0) {
                lftpd_log_error("write error at %ld/%ld", sent, total);
                fclose(file);
                return -1;
            }
            p += write_len;
            read_len -= write_len;
            sent += write_len;
        }
        /* Progress every 1MB */
        if((sent % (1024 * 1024)) < 4096) {
            lftpd_log_info("  RETR %ld/%ld (%ld%%)", sent, total,
                           total > 0 ? sent * 100 / total : 0);
        }
    }

    fclose(file);
    lftpd_log_info("RETR complete: %ld bytes", sent);

    return 0;
}

static int receive_file(int socket, const char *path) {
    FILE *file = fopen(path, "wb");
    if(file == NULL) {
        lftpd_log_error("failed to open file for write");
        return -1;
    }

    unsigned char buffer[4096];
    int err;
    while((err = read(socket, buffer, sizeof(buffer))) > 0) {
        if(fwrite(buffer, err, 1, file) != 1) {
            err = -1;
            break;
        }
    }

    fclose(file);

    if(err < 0) {
        return err;
    }

    /* Sync the upload to the SD card or IDE */
    if(!strncmp(path, "/sd/", 4))
        fs_fat_sync("/sd");
    else if(!strncmp(path, "/ide/", 5))
        fs_fat_sync("/ide");

    return 0;
}

static int cmd_cwd(lftpd_client_t *client, const char *arg) {
    if(arg == NULL || strlen(arg) == 0) {
        send_simple_response(client->socket, 550, STATUS_550);
        return -1;
    }

    char *path = lftpd_io_canonicalize_path(client->directory, arg);

    // make sure the path exists
    struct stat st;
    if(stat(path, &st) != 0) {
        send_simple_response(client->socket, 550, STATUS_550);
        free(path);
        return -1;
    }

    // make sure the path is a directory
    if(!S_ISDIR(st.st_mode)) {
        send_simple_response(client->socket, 550, STATUS_550);
        free(path);
        return -1;
    }

    free(client->directory);
    client->directory = path;
    send_simple_response(client->socket, 250, STATUS_250);

    return 0;
}

static int cmd_dele(lftpd_client_t *client, const char *arg) {
    if(arg == NULL || strlen(arg) == 0) {
        send_simple_response(client->socket, 550, STATUS_550);
        return -1;
    }

    char *path = lftpd_io_canonicalize_path(client->directory, arg);

    // make sure the path exists
    struct stat st;
    if(stat(path, &st) != 0) {
        send_simple_response(client->socket, 550, STATUS_550);
        free(path);
        return -1;
    }

    // make sure the path is a file
    if(!S_ISREG(st.st_mode)) {
        send_simple_response(client->socket, 550, STATUS_550);
        free(path);
        return -1;
    }

    remove(path);
    free(path);
    send_simple_response(client->socket, 250, STATUS_250);

    return 0;
}

static int cmd_epsv(lftpd_client_t *client, const char *arg) {
    // Reuse PASV port logic — no IPv6 on KOS
    send_simple_response(client->socket, 502, STATUS_502);
    return -1;
}

static int cmd_feat(lftpd_client_t *client, const char *arg) {
    send_multiline_response_begin(client->socket, 211, STATUS_211);
    send_multiline_response_line(client->socket, "PASV");
    send_multiline_response_line(client->socket, "SIZE");
    send_multiline_response_line(client->socket, "NLST");
    send_multiline_response_end(client->socket, 211, STATUS_211);
    return 0;
}

static int cmd_list(lftpd_client_t *client, const char *arg) {
    if(client->data_socket == -1) {
        send_simple_response(client->socket, 425, STATUS_425);
        return -1;
    }

    send_simple_response(client->socket, 150, STATUS_150);
    int err = send_list(client->data_socket, client->directory);
    close(client->data_socket);
    client->data_socket = -1;
    if(err == 0) {
        send_simple_response(client->socket, 226, STATUS_226);
    }
    else {
        send_simple_response(client->socket, 550, STATUS_550);
    }
    return 0;
}

static int cmd_nlst(lftpd_client_t *client, const char *arg) {
    if(client->data_socket == -1) {
        send_simple_response(client->socket, 425, STATUS_425);
        return -1;
    }

    send_simple_response(client->socket, 150, STATUS_150);
    int err = send_nlst(client->data_socket, client->directory);
    close(client->data_socket);
    client->data_socket = -1;
    if(err == 0) {
        send_simple_response(client->socket, 226, STATUS_226);
    }
    else {
        send_simple_response(client->socket, 550, STATUS_550);
    }
    return 0;
}

static int cmd_noop(lftpd_client_t *client, const char *arg) {
    send_simple_response(client->socket, 200, STATUS_200);
    return 0;
}

static int cmd_pass(lftpd_client_t *client, const char *arg) {
    send_simple_response(client->socket, 230, STATUS_230);
    return 0;
}

static int cmd_pasv(lftpd_client_t *client, const char *arg) {
    // Use explicit ports — KOS getsockname doesn't reliably
    // return the port after bind(0)
    int listener_socket = -1;
    uint16_t port = 0;

    if(client->data_socket >= 0) {
        close(client->data_socket);
        client->data_socket = -1;
    }

    for(int tries = 0; tries < 20; tries++) {
        port = g_pasv_port++;
        if(g_pasv_port > 60000) g_pasv_port = PASV_PORT_BASE;

        listener_socket = lftpd_inet_listen(port);
        if(listener_socket >= 0) break;
    }

    if(listener_socket < 0) {
        send_simple_response(client->socket, 425, STATUS_425);
        return -1;
    }

    uint8_t *ip = net_default_dev->ip_addr;
    send_simple_response(client->socket, 227, STATUS_227,
                         ip[0], ip[1], ip[2], ip[3],
                         (port >> 8) & 0xff, (port >> 0) & 0xff);

    lftpd_log_info("PASV waiting on port %d...", port);

    /* Wait for the client to open the data connection, but don't block forever */
    fd_set rfds;
    struct timeval tv = { .tv_sec = 30, .tv_usec = 0 };
    FD_ZERO(&rfds);
    FD_SET(listener_socket, &rfds);
    if(select(listener_socket + 1, &rfds, NULL, NULL, &tv) <= 0) {
        lftpd_log_error("timed out waiting for data connection");
        close(listener_socket);
        return -1;
    }

    int client_socket = accept(listener_socket, NULL, NULL);
    if(client_socket < 0) {
        lftpd_log_error("error accepting data connection");
        close(listener_socket);
        return -1;
    }
    lftpd_log_info("PASV data connection on port %d", port);

    close(listener_socket);
    client->data_socket = client_socket;

    return 0;
}

static int cmd_pwd(lftpd_client_t *client, const char *arg) {
    send_simple_response(client->socket, 257, "\"%s\"", client->directory);
    return 0;
}

static int cmd_quit(lftpd_client_t *client, const char *arg) {
    send_simple_response(client->socket, 221, STATUS_221);
    return -1;
}

static int cmd_retr(lftpd_client_t *client, const char *arg) {
    if(client->data_socket == -1) {
        send_simple_response(client->socket, 425, STATUS_425);
        return -1;
    }

    send_simple_response(client->socket, 150, STATUS_150);
    char *path = lftpd_io_canonicalize_path(client->directory, arg);
    lftpd_log_debug("send '%s'", path);
    int err = send_file(client->data_socket, path);
    free(path);
    close(client->data_socket);
    client->data_socket = -1;
    if(err == 0) {
        send_simple_response(client->socket, 226, STATUS_226);
    }
    else {
        send_simple_response(client->socket, 450, STATUS_450);
    }
    return 0;
}

static int cmd_size(lftpd_client_t *client, const char *arg) {
    if(!arg) {
        send_simple_response(client->socket, 550, STATUS_550);
        return 0;
    }

    char *path = lftpd_io_canonicalize_path(client->directory, arg);
    lftpd_log_debug("size %s", path);
    struct stat st;
    if(stat(path, &st) == 0) {
        send_simple_response(client->socket, 213, "%ld", (long)st.st_size);
    }
    else {
        send_simple_response(client->socket, 550, STATUS_550);
    }
    free(path);
    return 0;
}

static int cmd_stor(lftpd_client_t *client, const char *arg) {
    if(client->data_socket == -1) {
        send_simple_response(client->socket, 425, STATUS_425);
        return -1;
    }

    send_simple_response(client->socket, 150, STATUS_150);
    char *path = lftpd_io_canonicalize_path(client->directory, arg);
    lftpd_log_debug("receive '%s'", path);
    int err = receive_file(client->data_socket, path);
    free(path);
    close(client->data_socket);
    client->data_socket = -1;
    if(err == 0) {
        send_simple_response(client->socket, 226, STATUS_226);
    }
    else {
        send_simple_response(client->socket, 450, STATUS_450);
    }
    return 0;
}

static int cmd_syst(lftpd_client_t *client, const char *arg) {
    send_simple_response(client->socket, 215, "UNIX Type: L8");
    return 0;
}

static int cmd_type(lftpd_client_t *client, const char *arg) {
    send_simple_response(client->socket, 200, STATUS_200);
    return 0;
}

static int cmd_user(lftpd_client_t *client, const char *arg) {
    send_simple_response(client->socket, 230, STATUS_230);
    return 0;
}

static int handle_control_channel(lftpd_client_t *client) {
    size_t read_buffer_len = 512;
    char *read_buffer = malloc(read_buffer_len);

    if(send_simple_response(client->socket, 220, STATUS_220) != 0) {
        lftpd_log_error("error sending welcome message");
        goto cleanup;
    }
    while(true) {
        int line_len = lftpd_inet_read_line(client->socket, read_buffer, read_buffer_len);
        if(line_len != 0) {
            lftpd_log_error("error reading next command");
            goto cleanup;
        }

        // find the index of the first space
        int index;
        char *p = strchr(read_buffer, ' ');
        if(p != NULL) {
            index = p - read_buffer;
        }
        // if no space, use the whole string
        else {
            index = strlen(read_buffer);
        }

        // if the index is 5 or greater the command is too long
        if(index >= 5) {
            send_simple_response(client->socket, 500, STATUS_500);
            continue;
        }

        // copy the command into a temporary buffer
        char command_tmp[4 + 1];
        memset(command_tmp, 0, sizeof(command_tmp));
        memcpy(command_tmp, read_buffer, index);

        // upper case the command
        for(int i = 0; command_tmp[i]; i++) {
            command_tmp[i] = (char) toupper((int) command_tmp[i]);
        }

        // see if we have a matching function for the command, and if
        // so, dispatch it
        bool matched = false;
        for(int i = 0; commands[i].command; i++) {
            if(strcmp(commands[i].command, command_tmp) == 0) {
                char *arg = NULL;
                if(index < strlen(read_buffer)) {
                    arg = strdup(read_buffer + index + 1);
                    arg = lftpd_string_trim(arg);
                }
                commands[i].handler(client, arg);
                free(arg);
                matched = true;
                break;
            }
        }
        if(!matched) {
            send_simple_response(client->socket, 502, STATUS_502);
        }

        /* End the session only on QUIT */
        if(strcmp(command_tmp, "QUIT") == 0)
            goto cleanup;
    }

cleanup:
    free(read_buffer);
    close(client->socket);

    return 0;
}

/* Thread entry point for each FTP client connection */
static void *client_thread(void *arg) {
    lftpd_client_t *client = (lftpd_client_t *)arg;
    lftpd_log_info("session start");
    handle_control_channel(client);
    free(client->directory);
    free(client);
    lftpd_log_info("session end");
    return NULL;
}

int lftpd_start(const char *directory, int port, lftpd_t *lftpd) {
    memset(lftpd, 0, sizeof(lftpd_t));

    lftpd->directory = directory;
    lftpd->port = port;
    lftpd->server_socket = lftpd_inet_listen(port);
    if(lftpd->server_socket < 0) {
        lftpd_log_error("error creating listener");
        return -1;
    }

    int actual_port = lftpd_inet_get_socket_port(lftpd->server_socket);
    lftpd_log_info("listening on port %d...", actual_port);

    while(true) {
        lftpd_log_info("waiting for connection...");

        int client_socket = accept(lftpd->server_socket, NULL, NULL);
        if(client_socket < 0) {
            /* A failed accept() must not take the whole server down */
            lftpd_log_error("error accepting client socket");
            thd_sleep(100);
            continue;
        }

        lftpd_log_info("connection received...");

        lftpd_client_t *client = malloc(sizeof(lftpd_client_t));
        if(client == NULL) {
            lftpd_log_error("out of memory accepting client");
            close(client_socket);
            continue;
        }
        client->directory = strdup(directory);
        client->socket = client_socket;
        client->data_socket = -1;

        /* Spawn a detached thread per connection so FileZilla
           can open multiple control channels simultaneously */
        thd_create(1, client_thread, client);
    }

    close(lftpd->server_socket);

    return 0;
}

int lftpd_stop(lftpd_t *lftpd) {
    close(lftpd->server_socket);
    return 0;
}

/* Flush any cached FAT writes */
static void clean_exit(void) {
    fs_fat_sync("/sd");
    fs_fat_sync("/ide");
    exit(0);
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    uint32 y_pos = 20;

    /* Press START to exit; clean_exit() flushes the FAT cache first */
    cont_btn_callback(0, CONT_START, (cont_btn_callback_t)clean_exit);

    vid_clear(23, 86, 155);
    bfont_draw_str(vram_s + y_pos * 640 + 20, 640, true, "KOS FTP Server (lftpd)");
    y_pos += 24;

    printf("===========================\n");
    printf("KOS FTP Server (lftpd)\n");
    printf("===========================\n\n");

    fs_fat_init();

    /* Mount IDE */
    printf("Initializing ATA...\n");
    if(g1_ata_init() < 0) {
        printf("  g1_ata_init failed\n");
    }
    else {
        uint8_t pt;

        if(g1_ata_blockdev_for_partition(0, 1, &ide_bd, &pt) < 0) {
            printf("  ATA partition not found\n");
        }
        else {
            printf("  ATA partition type=0x%02x\n", pt);

            if(fs_fat_mount("/ide", &ide_bd, FS_FAT_MOUNT_READWRITE) < 0) {
                printf("  IDE mount failed\n");
            } else {
                printf("  Mounted /ide\n\n");
                bfont_draw_str(vram_s + y_pos * 640 + 20, 640, true, "IDE MOUNTED");
                y_pos += 24;
            }
        }
    }

    printf("Initializing SD...\n");
    if(sd_init() < 0) {
        printf("  sd_init failed\n");
    }
    else {
        uint8_t pt;

        if(sd_blockdev_for_partition(0, &sd_bd, &pt) < 0) {
            printf("  SD partition not found\n");
        }
        else {
            printf("  SD partition type=0x%02x\n", pt);

            if(fs_fat_mount("/sd", &sd_bd, FS_FAT_MOUNT_READWRITE) < 0) {
                printf("  SD mount failed\n");
            } else {
                printf("  Mounted /sd\n\n");
                bfont_draw_str(vram_s + y_pos * 640 + 20, 640, true, "SD MOUNTED");
                y_pos += 24;
            }
        }
    }

    /* Wait for DHCP / network to come up */
    printf("Waiting for network (%d ms)...\n", DHCP_WAIT_MS);
    thd_sleep(DHCP_WAIT_MS);

    uint8_t *ip = net_default_dev->ip_addr;
    printf("IP: %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
    bfont_draw_str_fmt(vram_s + y_pos * 640 + 20, 640, true,
                        "FTP: %d.%d.%d.%d:%d",
                        ip[0], ip[1], ip[2], ip[3], FTP_PORT);
    y_pos += 24;

    printf("Press START to exit.\n\n");
    bfont_draw_str(vram_s + y_pos * 640 + 20, 640, true, "Press START to exit");

    /* Start FTP server — blocks forever */
    lftpd_t lftpd;
    lftpd_start("/", FTP_PORT, &lftpd);

    for(;;) thd_sleep(1000);
    return 0;
}

static command_t commands[] = {
    { "CWD", cmd_cwd },
    { "DELE", cmd_dele },
    { "EPSV", cmd_epsv },
    { "FEAT", cmd_feat },
    { "LIST", cmd_list },
    { "NLST", cmd_nlst },
    { "NOOP", cmd_noop },
    { "PASS", cmd_pass },
    { "PASV", cmd_pasv },
    { "PWD", cmd_pwd },
    { "QUIT", cmd_quit },
    { "RETR", cmd_retr },
    { "SIZE", cmd_size },
    { "STOR", cmd_stor },
    { "SYST", cmd_syst },
    { "TYPE", cmd_type },
    { "USER", cmd_user },
    { NULL, NULL },
};
