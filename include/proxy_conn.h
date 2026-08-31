#ifndef PROXY_CONN_H
#define PROXY_CONN_H
#include <sys/epoll.h>
#include "proxy.h"
#include "route_table.h"

void handle_read_headers(proxy_conn_t *conn, proxy_conn_t **conn_table, route_profile *route_table, int route_count, int epoll_fd);
void handle_connecting_backend(proxy_conn_t *conn, int epoll_fd);
void handle_piping(proxy_conn_t *conn, struct epoll_event event, int epoll_fd);
void handle_closing(proxy_conn_t *conn, proxy_conn_t **conn_table, int epoll_fd, int *activec);

#endif