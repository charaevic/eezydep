#ifndef HTTP_PARSE
#define HTTP_PARSE
#include <stddef.h>
typedef struct{
    char method[8];
    char path[2048];
    char host[256];
    int content_length;
    int keep_alive;
} http_request_t;
int http_parse_request(const char *raw, size_t len, http_request_t * req);
#endif