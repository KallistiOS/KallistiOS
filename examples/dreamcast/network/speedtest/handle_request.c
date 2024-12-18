
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "speedtest.h"

/*
   This file defines the handle_request function, which processes HTTP requests 
   received by the speed test server. It parses the request, determines the HTTP 
   method (GET or POST), and routes the request to the appropriate handler based 
   on the requested path. It supports GET requests to serve files such as 
   index.html and handles download and upload test endpoints to measure network 
   speed.
 
   Key paths:
   - "/": serves the speed test HTML page (index.html).
   - "/download-test?size=": initiates a download test of the requested size.
   - "/upload-test": handles data uploads during an upload test.
 */


bool exact_match(const char *path, const char *pattern);
bool prefix_match(const char *path, const char *pattern);

int send_ok(http_state_t *h, const char *ct);
void send_code(int socket, uint16_t code, const char *message);

#define BUFSIZE 1024
#define REQUEST_LINE_SIZE 160
#define HEADER_BUF_SIZE  512

void *handle_request(void *p) {
    char request_line[REQUEST_LINE_SIZE] = {0};
    char *buf_ptr = request_line;
    char *path_end;
    size_t total_bytes = 0;
    http_state_t *hr = (http_state_t *)p;

    /* Read the max we expect the request line to be */
    total_bytes = recv(hr->socket, request_line, REQUEST_LINE_SIZE-1, MSG_NONE);
    if(total_bytes <= 0) {
        send_code(hr->socket, 400, "Bad Request.");
        goto process_request_out;
    }
    request_line[total_bytes] = '\0';

    /* If first byte is 'G' we have GET */
    if(*buf_ptr == 'G') {
        hr->method = METHOD_GET;
        buf_ptr += 4; /* Move to the start of path */
    } else if(*buf_ptr == 'P' && *(buf_ptr+1) == 'O') {
        hr->method = METHOD_POST;
        buf_ptr += 5; /* Move to the start of path */
    } else {
        send_code(hr->socket, 501, "Method not implemented.");
        goto process_request_out;
    }

    /* Find the end of path and put \0. */
    path_end = strchr(buf_ptr, ' ');
    if(!path_end) {
        send_code(hr->socket, 414, "Request-URI Too Long.");
        goto process_request_out;
    }
    *path_end = '\0'; /* Replace space with terminator */
    hr->path = buf_ptr;
    buf_ptr = path_end + 1;

    printf("%s\n", hr->path);

    if(hr->method == METHOD_POST) {
        char *content_length_key = "Content-Length: ";
        char *cl_key_ptr = content_length_key;

        char *body_start_key = "\r\n\r\n";
        char *bs_key_ptr = body_start_key;

        char buf[HEADER_BUF_SIZE] = {0};

        char *found;
        char *buf_start = request_line;
        bool got_content_length = false;
        size_t bytes_left = total_bytes - (buf_ptr - buf_start);

        printf("RL: %s\n", buf_ptr);

        /* Look for Content-Length header(optional) and start of the body */
        while(true) {
            /* Iterate over each byte trying to match */
            while(bytes_left > 0) {
                /* Look for optional Content-Length header */
                if(!got_content_length) {
find_length:
                    if(*buf_ptr == *cl_key_ptr) {
                        while(bytes_left-- && *cl_key_ptr != '\0' && *buf_ptr++ == *cl_key_ptr++);
                        if(*cl_key_ptr == '\0') { /* We found Content-Length key */
                            /* Get the count */
                            hr->rem_content_length = atoi(buf_ptr);
                            /* Check that we grabbed the full number */
                            found = strstr(buf_ptr, "\r\n");
                            if(found) {
                                buf_ptr = found;
                                got_content_length = true;
                                
                                goto find_body;
                            }
                        }
                        else if(bytes_left > 0) { /* Start over */
                            cl_key_ptr = content_length_key;

                            /* Rewind back one char */
                            buf_ptr--;
                            bytes_left++;
                            /* Fall through to find_body */
                        } 
                        else /* bytes_left == 0 */
                            goto refresh_buffer;
                    }
                }
find_body:
                /* Look for body start */
                if(*buf_ptr == *bs_key_ptr) {
                    while(bytes_left-- && *bs_key_ptr != '\0' && *buf_ptr++ == *bs_key_ptr++);
                    if(*bs_key_ptr == '\0') { /* We found body start key */
                        hr->body = buf_ptr;
                        /* How much of the body do we already have? */
                        hr->read_content_length = total_bytes - (hr->body - buf_start);
                        if(got_content_length)
                            hr->rem_content_length -= hr->read_content_length;
                        
                        goto done_parsing;
                    }
                    else if(bytes_left > 0) /* Start over */ {
                        bs_key_ptr = body_start_key;

                        if(!got_content_length) {
                            /* Rewind back one char */
                            buf_ptr--;
                            bytes_left++;
                            goto find_length;
                        }
                    }
                    else /* bytes_left == 0 */
                        goto refresh_buffer;
                }

                /* Should only hit here if neither matched at the starting character */
                buf_ptr++;
                bytes_left--;
            }

refresh_buffer:
            /* We get here when we are done searching through current buffer */
            total_bytes = recv(hr->socket, buf, HEADER_BUF_SIZE-1, MSG_NONE);
            if(total_bytes <= 0) {
                send_code(hr->socket, 400, "Bad Request.");
                goto process_request_out;
            }
            buf[total_bytes] = '\0';
            buf_start = buf;
            buf_ptr = buf;
            bytes_left = total_bytes;

            /* If we have already found Content-Length, skip searching it again */
            if(got_content_length)
                goto find_body;
        }
    }

done_parsing:
    char response_buf[BUFSIZE];
    if(hr->method == METHOD_GET) {
        file_t file = -1;
        uint32_t offset;
        int cnt;
        int wrote;

        /* index.html */
        if(exact_match(hr->path, "") || exact_match(hr->path, "/")) {
            file = fs_open("/rd/index.html", O_RDONLY);

            send_ok(hr, "text/html");
            while((cnt = fs_read(file, response_buf, BUFSIZE)) != 0) {
                offset = 0;

                while(cnt > 0) {
                    wrote = send(hr->socket, response_buf + offset, cnt, MSG_NONE);

                    if(wrote <= 0)
                        break;

                    cnt -= wrote;
                    offset += wrote;
                }
            }

            fs_close(file);
            goto process_request_out;
        }
        else if(prefix_match(hr->path, "/download-test")) {
            char *size_str;
            size_t size;

            /* Extract size parameter from path */
            size_str = strstr(hr->path, "size=");
            if(size_str) {
                size = strtoul(size_str + 5, NULL, 10);
                if(size <= 0 || size > 16*1024*1024) {
                     /* Send ERROR Code with reason */
                    send_code(hr->socket, 400, "GET download: 'size' is out of range (1 - 16*1024*1024)");
                }

                send_ok(hr, "application/octet-stream");
                uintptr_t data = 0x8000000;
                offset = 0;
                while(size > 0) {
                    wrote = send(hr->socket, (uint8_t *)data + offset, size, MSG_NONE);

                    if(wrote <= 0)
                        break;

                    size -= wrote;
                    offset += wrote;
                }

                goto process_request_out;
            } else {
                /* Send ERROR Code with reason */
                send_code(hr->socket, 400, "GET download: Missing required params (size)");
            }
        }
    }
    else { /* POST */
        if(exact_match(hr->path, "/upload-test")) {
            while(hr->rem_content_length) {
                total_bytes = recv(hr->socket, response_buf, HEADER_BUF_SIZE - 1, MSG_NONE);

                hr->rem_content_length -= total_bytes;
            }
            send_code(hr->socket, 204, "");

            goto process_request_out;
        }
    }

    /* If no handler was found */
    send_code(hr->socket, 404, "Invalid request or file not found.");

process_request_out:
    /* Clean up our http state and all associated allocated memory */
    close(hr->socket);
    free(hr);

    /* We're now done with this thread. */
    return NULL;
}

bool exact_match(const char *path, const char *pattern) {
    if (strlen(path) != strlen(pattern))
        return false;

    return strcmp(path, pattern) == 0;
}

bool prefix_match(const char *path, const char *pattern) {
    return strncmp(path, pattern, strlen(pattern)) == 0;
}

int send_ok(http_state_t *h, const char *ct) {
    char buffer[512];

    sprintf(buffer, "HTTP/1.0 200 OK\r\nContent-type: %s\r\nConnection: close\r\n\r\n", ct);
    send(h->socket, buffer, strlen(buffer), MSG_NONE);

    return 0;
}

void send_code(int socket, uint16_t code, const char *message) {
    /* Send HTTP response header */
    char *buf;
    size_t buf_size;

    /* Calculate size of message */
    buf_size = snprintf(NULL, 0,
                    "HTTP/1.1 %d %s\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: %zu\r\n"
                    "Connection: close\r\n"
                    "\r\n"
                    "%s",
                    code, message, strlen(message), message);

    /* Allocate buf on the stack */
    buf = (char *)alloca(buf_size + 1); /* Null terminator */

    buf_size = snprintf(buf, buf_size + 1,
                    "HTTP/1.1 %d %s\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: %zu\r\n"
                    "Connection: close\r\n"
                    "\r\n"
                    "%s",
                    code, message, strlen(message), message);

    send(socket, buf, buf_size, MSG_NONE);
}
