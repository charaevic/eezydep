#define _POSIX_C_SOURCE 199309L
#include "proxy.h"
#include "http_parse.h"
#include "route_table.h"
#include "log.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <limits.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
void transfer(proxy_conn_t*, int, int, int, wbuf_t*);
void send_http_error(int, int, const char*);
int flush_wbuf(int, wbuf_t*);
void handle_read_headers(proxy_conn_t *conn, proxy_conn_t **conn_table, route_profile * route_table, int route_count, int epoll_fd){
    int numbytes;
    /* receive into the buffer, taking into account that the header might be split, so use pointer arithmetic
    to keep track of the length of buffer occupied then simply append to that until we are certain to have the \r\n\r\n */
    if((numbytes = recv(conn->client_fd, (conn->recv_buf)+(conn->recv_len), sizeof(conn->recv_buf)-(conn->recv_len), 0))>0){
        conn->last_activity = time(NULL);
        conn->recv_len+=numbytes;
        conn->recv_buf[conn->recv_len] = '\0';
        char * end = strstr(conn->recv_buf, "\r\n\r\n");
        //If end chars not in stream return and go back to epoll (continuing condition)
        if(end == NULL){return;}
        //parse HTTP
        if(http_parse_request(conn->recv_buf, conn->recv_len, &conn->req)!=-1){
            route_profile* lookup_res = route_lookup(route_table, route_count, conn->req.host);
            if(lookup_res == NULL || !lookup_res->healthy){
                
                conn->state = STATE_CLOSING;
                send_http_error(conn->client_fd, lookup_res ? 503 : 502, lookup_res ? "Service Unavailable" : "Bad Gateway");
                
                return;
            } else {
                //route found, non-blck socket and connect to backend
                strncpy(conn->backend_addr, lookup_res->backend_addr, sizeof(conn->backend_addr)-1);
                conn->backend_port = lookup_res->backend_port;
                //fresh socket
                int new_sck = socket(AF_INET, SOCK_STREAM, 0);
                //fill in address
                struct sockaddr_in backend_addr;
                backend_addr.sin_family = AF_INET;
                backend_addr.sin_port = htons(lookup_res->backend_port);
                inet_pton(AF_INET, lookup_res->backend_addr, &backend_addr.sin_addr);
                
                fcntl(new_sck, F_SETFL, O_NONBLOCK);
                struct epoll_event new_event = {.events = EPOLLOUT, .data.fd = new_sck};
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_sck, &new_event);

                //connect non-blck socket
                connect(new_sck, (struct sockaddr *) &backend_addr, sizeof(backend_addr));
                clock_gettime(CLOCK_MONOTONIC, &conn->start_time);
                conn->state = STATE_CONN_BACKEND;
                conn->backend_fd = new_sck;
                conn_table[new_sck] = conn; 
            }
        }
    }
}

void handle_connecting_backend(proxy_conn_t *conn, int epoll_fd){
    int error = 0;
    //get 
    socklen_t errlen = sizeof(error);
    //write the value of SO_ERROR opt into the error variable then check if its 0 (connected) or not
    getsockopt(conn->backend_fd, SOL_SOCKET, SO_ERROR, &error, &errlen);
    if (error !=0){
        conn->state = STATE_CLOSING;
        send_http_error(conn->client_fd, 502, "Bad Gateway");
        return;
    } else{
        //forward whatever is in conn pointer (recv_buf)
        //send it to conn->backend_fd
        if(send(conn->backend_fd, conn->recv_buf, conn->recv_len, 0) == -1){
            conn->state = STATE_CLOSING;
            return;
        }
        conn->last_activity = time(NULL);

        //switch both backend and client fd to EPOLLIN
        conn->state = STATE_PIPING;
        struct epoll_event mod_event = {.events = EPOLLIN, .data.fd = conn->client_fd};
        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, conn->client_fd, &mod_event);
        mod_event.data.fd = conn->backend_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, conn->backend_fd, &mod_event);
        return;    
    }
}

void handle_piping(proxy_conn_t *conn, struct epoll_event event, int epoll_fd){
    if(event.events & EPOLLOUT){
        //another conditional partial send
        int partial;
        if(conn->backend_fd == event.data.fd) partial = flush_wbuf(conn->backend_fd, &conn->backend_wbuf); 
        else partial = flush_wbuf(conn->client_fd, &conn->client_wbuf);
        if(partial == 1){return;}
        else if(partial == 0){
            //remove epollout
            struct epoll_event mod_event = {.events = EPOLLIN, .data.fd = event.data.fd};
            epoll_ctl(epoll_fd, EPOLL_CTL_MOD, event.data.fd, &mod_event);
            return;
        }
        else{
            conn->state = STATE_CLOSING;
            return;
        }

    } else{
        if(event.data.fd == conn->backend_fd){
            transfer(conn, event.data.fd, conn->client_fd, epoll_fd, &conn->client_wbuf);
        } else if (event.data.fd == conn-> client_fd){
            transfer(conn, event.data.fd, conn->backend_fd, epoll_fd, &conn->backend_wbuf);
        }
        //epoll should only ever contain backend and client fds any other incoming fds are disregarded
    }
}

void handle_closing(proxy_conn_t* conn, proxy_conn_t **conn_table, int epoll_fd, int *activec){
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, conn->client_fd, NULL);
    conn_table[conn->client_fd] = NULL;
    close(conn->client_fd);
    if (conn->backend_fd != -1){
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, conn->backend_fd, NULL);
        close(conn->backend_fd);
        conn_table[conn->backend_fd] = NULL;
    }
    (*activec)--;
    log_access(conn);
    free(conn);
    return;
}

void transfer(proxy_conn_t* conn, int triggered_fd, int target_fd, int epoll_fd, wbuf_t* wb){
    char buf[8192];
    int bytes_received = recv(triggered_fd, buf, sizeof(buf), 0);

    if(bytes_received == 0){
        conn->state = STATE_CLOSING;
        return;
    
    }else if(bytes_received == -1){
        if(errno == EAGAIN || errno == EWOULDBLOCK){
            return;
        } else{
            conn->state = STATE_CLOSING;
            return;
        }
    } else{
        if (triggered_fd == conn->backend_fd && !conn->first_response){
            conn->first_response = 1;
            if (bytes_received>=12){
                //if first resp, it will necessarily be an HTTP response containing status so we can safely record
                char status_str[4] = {buf[9], buf[10], buf[11], '\0'};
                conn->response_status = atoi(status_str);
            }
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            conn->latency_ms = ((now.tv_sec - conn->start_time.tv_sec))* 1000 + (now.tv_nsec - conn->start_time.tv_nsec)/1000000;
        }
        //enqueue
        memcpy(wb->data + wb->wpos, buf, bytes_received);
        wb->wpos += bytes_received;
        int rem = flush_wbuf(target_fd, wb);

        if (target_fd == conn->client_fd){
            conn->bytes_sent +=bytes_received;
        }

        if (rem == 1) {
        // Partial send — register recepient to EPOLLOUT to retry later
            struct epoll_event mod_event = {.events = EPOLLOUT | EPOLLIN, .data.fd = target_fd};
            epoll_ctl(epoll_fd, EPOLL_CTL_MOD, target_fd, &mod_event);
        } else if(rem ==-1){
            conn->state = STATE_CLOSING;
            return;
        }
        conn->last_activity = time(NULL);
    }
}
int flush_wbuf(int target_fd, wbuf_t *wb){
    ssize_t sent = send(target_fd, wb->data+wb->rpos, wb->wpos-wb->rpos, 0);
    if (sent == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return 1;
        return -1;
    }
    else if (sent >0) wb->rpos +=sent;
    
    //compact
    if (wb->rpos > sizeof(wb->data) / 2) {
            memmove(wb->data, wb->data + wb->rpos, wb->wpos - wb->rpos);
            wb->wpos -= wb->rpos;
            wb->rpos = 0;
        }

    if(wb->rpos < wb->wpos) return 1; else return 0;
}

void send_http_error(int spec_fd, int status_code, const char *reason){
    char header[1024];
    snprintf(header, sizeof(header), "HTTP/1.1 %d %s\r\nContent-Length: %d\r\n\r\n%s", status_code, reason, (int)strlen(reason), reason);
    int sent = send(spec_fd, header, strlen(header), 0);
    return;
}

