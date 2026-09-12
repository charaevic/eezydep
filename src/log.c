#include <stdio.h>
#include "proxy.h"
#include <time.h>
void log_access(proxy_conn_t* conn){
    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);
    char timebuf[64];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", timeinfo);
    fprintf(stderr, "%s | %s | %s %s | -> %s:%d | %d | %ldms | %zuB\n",
        timebuf,
        conn->req.host,
        conn->req.method,
        conn->req.path,
        conn->backend_addr,
        conn->backend_port,
        conn->response_status,
        conn->latency_ms,
        conn->bytes_sent
    );
}