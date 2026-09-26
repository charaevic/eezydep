#ifndef PROXY
#define PROXY
#include "http_parse.h"
#include <time.h>
typedef enum{
    STATE_READ_HEADER,
    STATE_CONN_BACKEND,
    STATE_PIPING,
    STATE_CLOSING
} conn_state_t;
typedef struct{
    char data[65536];
    size_t rpos; //next byte to send
    size_t wpos; // next byte to fill
} wbuf_t;

typedef struct {
    int client_fd;
    int backend_fd;
    int paused_fd;
    conn_state_t state;
    char recv_buf[8192];
    size_t recv_len;
    http_request_t req;
    wbuf_t client_wbuf;
    wbuf_t backend_wbuf;
    time_t last_activity;
    struct timespec start_time;
    long latency_ms;
    int response_status;
    int first_response; //flag
    size_t bytes_sent;
    char backend_addr[64];
    int backend_port;

} proxy_conn_t;

#endif