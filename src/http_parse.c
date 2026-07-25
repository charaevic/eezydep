#include "http_parse.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
int http_parse_request(const char *raw, size_t len, http_request_t* req){
    //we have a string pointer
    char buf[len+1];
    buf[len]='\0';
    char buf2[len+1];
    buf2[len] = '\0';
    strncpy(buf, raw, len);
    strncpy(buf2, raw, len);
    
    char* request_type = strtok(buf, " ");
    if(request_type == NULL){return -1;}
    char* path = strtok(NULL, " ");
    if(path == NULL){return -1;}

    //Fill in method and path sections with strings (null terminated)
    strncpy(req->method, request_type, sizeof(req->method)-1);
    req->method[sizeof(req->method)-1] = '\0';
    strncpy(req->path, path, sizeof(req->path)-1);
    req->path[sizeof(req->path)-1] = '\0';

    //Extract host string
    char* hosts = strstr(buf2, "Host: ");
    if(hosts == NULL){return -1;}
    char* hoste = strstr(hosts, "\r\n");
    if(hoste == NULL){return -1;}
    size_t length = hoste - (hosts+6);
    strncpy(req->host, hosts+6, length);
    req->host[length] = '\0';

    //content-length
    char* cls = strstr(buf2, "Content-Length: ");
    if (cls == NULL) {
        req->content_length = -1;
    }else{
        req->content_length = atoi(cls+16);
    }
    //connection type
    char* cts = strstr(buf2, "Connection: ");
    if (cts == NULL) {
        req->keep_alive = 0;
    } else {
        int is_keep_alive = strncmp(cts+12, "keep-alive", 10);
        if(!(is_keep_alive)){
            req->keep_alive = 1;
        }else{
            req->keep_alive = 0;
        }
    }
    return 0;
}