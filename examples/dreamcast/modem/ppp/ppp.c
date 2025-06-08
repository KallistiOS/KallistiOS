/* KallistiOS ##version##

   ppp.c
   Copyright (C)2022 Luke Benstead

   Modem PPP example (intended for DreamPi connection)
*/

#include <stdio.h>
#include <kos.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <kos/net.h>
#include <ppp/ppp.h>
#include <time.h>

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_NET);


int main(int argc, const char* argv[]) {
    char buffer[1024];

    int err;
    struct addrinfo *ai;
    struct addrinfo hints;

    const struct sockaddr* addr_ptr;
    socklen_t addr_len;
    struct sockaddr_in addr4;
    struct sockaddr_in6 addr6;

    time_t start;
    int total_bytes = 0;

    if(!modem_init()) {
        printf("modem_init failed!\n");
        return 1;
    }

    ppp_init();

    printf("Dialing connection\n");
    err = ppp_modem_init("555", 0, NULL);
    if(err != 0) {
        printf("Couldn't dial a connection (%d)\n", err);
        return 1;
    }

    printf("Establishing PPP link\n");
    ppp_set_login("dream", "cast");

    err = ppp_connect();
    if(err != 0) {
        printf("Couldn't establish PPP link (%d)\n", err);
        return 1;
    }

    /* Make a DNS lookup for google */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    err = getaddrinfo("google.com", "80", &hints, &ai);
    if(err != 0) {
        printf("Unable to perform DNS lookup (%d)\n", err);
        return 1;
    }

    /* Get the first address v4 or v6, whatever */
    if(ai->ai_family == AF_INET) {
        addr4 = *(struct sockaddr_in *) ai->ai_addr;
        addr4.sin_family = AF_INET;
        addr4.sin_port = htons(80);
        addr_ptr = (const struct sockaddr*) &addr4;
        addr_len = sizeof(struct sockaddr_in);
    } else if(ai->ai_family == AF_INET6) {
        addr6 = *(struct sockaddr_in6 *) ai->ai_addr;
        addr6.sin6_family = AF_INET6;
        addr6.sin6_port = htons(80);
        addr_ptr = (const struct sockaddr*) &addr6;
        addr_len = sizeof(struct sockaddr_in6);
    } else {
        printf("Unexpected IP family\n");
        return 1;
    }

    freeaddrinfo(ai);

    /* Make a POST request to Google to make sure things are working */
    const char* req = "POST / HTTP/1.1\r\nHost: www.google.com\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: 27\r\n\r\nfield1=value1&field2=value2\r\n\r\n";

    int s = socket(AF_INET, SOCK_STREAM, 0);
    err = connect(s, addr_ptr, addr_len);

    for(int i = 0; i < 10; ++i) {
        sleep(1);
        printf("Sending request: %d. Response follows: \n\n\n\n", i);
        int sent = send(s, req, strlen(req), 0);
        if(sent == -1) {
            printf("Error sending request\n");
            return 1;
        } else if(sent != strlen(req)) {
            printf("Error sending full request\n");
            return 1;
        }

        start = time(NULL);
        total_bytes = 0;
        while(1) {
            int bytes = recv(s, buffer, 1024, MSG_DONTWAIT);
            if(bytes <= 0) {
                if(total_bytes) {
                    /* We received something previously so we're done */
                    break;
                } else if(time(NULL) > start + 30) {
                    printf("Timeout while waiting for response\n");
                    usleep(10);
                    break;
                }
            } else {
                total_bytes += bytes;
                printf("%s", buffer);
            }
        }
    }

    return 0;
}
