#include "health.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
void check_backend_health(route_profile *route){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(route->backend_port);
    inet_pton(AF_INET, route->backend_addr, &addr.sin_addr);

    struct timeval tv = {.tv_sec=2, .tv_usec = 0};
    //set send and recv timeouts 
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0){
        route->healthy = 1;
    } else{
        route->healthy = 0;
    }
    close(sock);
    route->last_check = time(NULL);


}