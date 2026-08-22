#ifndef PROXY
#define PROXY
#include "http_parse.h"
typedef enum{
    STATE_READ_HEADER,
    STATE_CONN_BACKEND,
    STATE_PIPING,
    STATE_CLOSING
} conn_state_t;


typedef struct {
    int client_fd;
    int backend_fd;
    conn_state_t state;
    char recv_buf[8192];
    size_t recv_len;
    http_request_t req;
} proxy_conn_t;

#endif