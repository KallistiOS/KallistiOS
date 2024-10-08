
#include <kos/thread.h>

#include <sys/socket.h>
#include <netinet/in.h>

typedef enum http_method {
    METHOD_GET = 1,
    METHOD_POST = 2,
} http_method_t;

typedef struct http_state {
    struct sockaddr_storage client;
    int                 socket;
    http_method_t       method;
    char                *path;
    /* POST */
    char                *body;
    /* read_content_length + rem_content_length is equal
     to the content-length attribute */
    size_t              read_content_length;
    size_t              rem_content_length;
} http_state_t;

#define MSG_NONE  0
#define DETACHED_THREAD 1

void *server_thread(void *p);
void *handle_request(void *p);
