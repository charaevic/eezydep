#include "proxy.h"
#include "http_parse.h"
#include "route_table.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <limits.h>
#include <sys/stat.h>
#include <fcntl.h>
void handle_read_headers(proxy_conn_t *conn, route_profile * route_table, int route_count, int epoll_fd){
    int numbytes;
    if((numbytes = recv(conn->client_fd, (conn->recv_buf)+(conn->recv_len), sizeof(conn->recv_buf)-(conn->recv_len), 0))>0){
        conn->recv_len+=numbytes;
        char * end = strstr(conn->recv_buf, "\r\n\r\n");
        //If end chars not in stream return and go back to epoll
        if(end == NULL){return;}
        //parse HTTP
        if(http_parse_request(conn->recv_buf, conn->recv_len, &conn->req)!=-1){
            route_profile* lookup_res = route_lookup(route_table, route_count, conn->req.host);
            if(lookup_res == NULL){
                conn->state = STATE_CLOSING;
                const char *bad_gw = "HTTP/1.1 502 Bad Gateway\r\n\r\n";
                send(conn->client_fd, bad_gw, strlen(bad_gw), 0);
                return;
            } else {
                //route found, non-blck socket and connect to backend
                //fresh socket
                int new_sck = socket(AF_INET, SOCK_STREAM, 0);
                //fill in address
                struct sockaddr_in backend_addr;
                backend_addr.sin_family = AF_INET;
                backend_addr.sin_port = htons(lookup_res->backend_port);
                inet_pton(AF_INET, lookup_res->backend_addr, &backend_addr.sin_addr);
                
                fcntl(new_sck, F_SETFL, O_NONBLOCK);
                struct epoll_event new_event = {.events = EPOLLOUT, .data.ptr = conn};
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_sck, &new_event);

                //connect non-blck socket
                connect(new_sck, (struct sockaddr *) &backend_addr, sizeof(backend_addr));
                conn->state = STATE_CONN_BACKEND;
                conn->backend_fd = new_sck;



            }
        }


    }
}

void handle_connecting_backend(proxy_conn_t *conn, int triggered_fd){
    
}