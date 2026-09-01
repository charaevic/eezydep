#include "proxy.h"
#include "http_parse.h"
#include "route_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <limits.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#define PORT "8000"
#define BACKLOG 128
volatile sig_atomic_t shutdown_flag = 0;
void handle_signal(int sig){
    (void)sig;
    shutdown_flag =1;
}
void event_loop(route_profile *route_table, int route_count, int listen_sock);
int main(void){
    signal(SIGTERM, handle_signal);
    signal(SIGHUP, handle_signal);
    signal(SIGINT, handle_signal);
    
    char* path = "./src/config/routes.conf";
    route_profile table[64];
    int routes_loaded = route_load(path, table, 64);
    if (routes_loaded <= 0) {
        printf("Failed to load routes from %s\n", path);
        return 1;
    }
    //listen socket setup
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    getaddrinfo(NULL, PORT, &hints, &res);
    int server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (bind(server_fd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("bind failed");
        return 1;
    }
    printf("Bound to port %s\n", PORT);
    listen(server_fd, BACKLOG);
    freeaddrinfo(res);


    event_loop(table, routes_loaded, server_fd);
    close(server_fd);
    return 0;

}