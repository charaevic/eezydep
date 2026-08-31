#include "proxy.h"
#include "http_parse.h"
#include "route_table.h"
#include "proxy_conn.h"
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
#define MAXEVENTS 64
#define MAXCONNECTIONS 1024
extern volatile sig_atomic_t shutdown_flag;
void event_loop(route_profile * route_table, int route_count, int listen_sock){
    //maxconn
    int active_connections = 0;
    //create epoll
    int epoll_fd = epoll_create1(0);
    //add the listening socket in (socket is created in the main file)
    struct epoll_event listener = {.events = EPOLLIN, .data.fd = listen_sock};
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_sock, &listener);
    //event array
    struct epoll_event events[MAXEVENTS];
    proxy_conn_t * conn_table[65536] = {NULL};

    while(!shutdown_flag){
        //wait for epoll events then loop thru them 
        int n = epoll_wait(epoll_fd, events, MAXEVENTS, -1);
        for(int i = 0; i< n; i++){
            int event_fd = events[i].data.fd;


            if (event_fd == listen_sock) {
                int new_client = accept(listen_sock, NULL, NULL);
                //block new conns if over lim
                if(active_connections >= MAXCONNECTIONS){
                    printf("Mi bomboclat too many\n"); //REMOVE==================================================================
                    fflush(stdout);
                    close(new_client);
                    continue;
                }
                printf("Got new conn");
                fflush(stdout);
                active_connections++;
                proxy_conn_t * new_conn = calloc(1, sizeof(proxy_conn_t));
                //handle malloc err
                new_conn->backend_fd = -1;
                new_conn->client_fd = new_client;
                new_conn->state = STATE_READ_HEADER;
                conn_table[new_client] = new_conn;
                //add this conn to epoll and set ptr to the new conn we made
                struct epoll_event new_event = {.events = EPOLLIN, .data.fd = new_client};
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD,  new_client, &new_event);
            } else {
                proxy_conn_t *conn = conn_table[event_fd];
                switch (conn->state) {
                    case STATE_READ_HEADER:     handle_read_headers(conn, conn_table, route_table, route_count, epoll_fd); break;
                    case STATE_CONN_BACKEND:    handle_connecting_backend(conn, epoll_fd); break;
                    case STATE_PIPING:          handle_piping(conn, events[i], epoll_fd); break;
                    case STATE_CLOSING:         handle_closing(conn, conn_table, epoll_fd, &active_connections); break;
                }
            }
        }
    }
    //Shutdown
    printf("Got shutdown: \n");
    fflush(stdout);

    for(int i = 0; i < 65536; i++){
        if (conn_table[i]!=NULL){
            handle_closing(conn_table[i], conn_table, epoll_fd, &active_connections);
        }
    }


}