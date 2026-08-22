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
void transfer(proxy_conn_t*, int, int);
void send_bad_gw(int);
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
                send_bad_gw(conn-> client_fd);
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

void handle_connecting_backend(proxy_conn_t *conn, int epoll_fd){
    int error = 0;
    socklen_t errlen = sizeof(error);
    getsockopt(conn->backend_fd, SOL_SOCKET, SO_ERROR, &error, &errlen);
    if (error !=0){
        conn->state = STATE_CLOSING;
        send_bad_gw(conn->client_fd);
        return;
    } else{
        //forward whatever is in conn pointer (recv_buf)
        //send it to conn->backend_fd
        if(send(conn->backend_fd, conn->recv_buf, conn->recv_len, 0) == -1){
            conn->state = STATE_CLOSING;
            return;
        }

        //switch both fd to EPOLLIN
        conn->state = STATE_PIPING;
        struct epoll_event mod_event = {.events = EPOLLIN, .data.ptr = conn};
        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, conn->client_fd, &mod_event);
        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, conn->backend_fd, &mod_event);
        return;
        


    }
}

void handle_piping(proxy_conn_t *conn, int triggered_fd){
    if(triggered_fd == conn->backend_fd){
        transfer(conn, triggered_fd, conn->client_fd);
    } else if (triggered_fd == conn-> client_fd){
        transfer(conn, triggered_fd, conn->backend_fd);
    }
    //epoll should only ever contain backend and client fds any other incoming fds are disregarded
}

void handle_closing(proxy_conn_t* conn, int epoll_fd){
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, conn->client_fd, NULL);
    close(conn->client_fd);
    if (conn->backend_fd != -1){
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, conn->backend_fd, NULL);
        close(conn->backend_fd);
    }
    free(conn);
    return;
}

void transfer(proxy_conn_t* conn, int triggered_fd, int target_fd){
    char buf[8192];
        int n = recv(triggered_fd, buf, sizeof(buf), 0);
        if(n == 0){
            conn->state = STATE_CLOSING;
            return;
        }

        if(send(target_fd, buf, n, 0) == -1){
            conn->state = STATE_CLOSING;
            return;
        }
        return;
}
void send_bad_gw(int spec_fd){
    const char *bad_gw = "HTTP/1.1 502 Bad Gateway\r\n\r\n";
    send(spec_fd, bad_gw, strlen(bad_gw), 0);
    return;
}