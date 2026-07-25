#include <assert.h>
#include <string.h>
#include "http_parse.h"
#include <stdio.h>

void test_basic_get(){
    const char *raw = "GET /index.html HTTP/1.1\r\nHost: alice.eezydep.org\r\n\r\n";
    http_request_t req;
    assert(http_parse_request(raw, strlen(raw), &req) == 0);
    assert(strcmp(req.method, "GET") == 0);
    assert(strcmp(req.path, "/index.html") == 0);
    assert(strcmp(req.host, "alice.eezydep.org") == 0);
    printf("PASS: test_basic_get\n");
}
void test_full_get(){
    http_request_t req;
    const char *raw = "GET /index.html HTTP/1.1\r\nHost: alice.eezydep.org\r\nContent-Length: 10\r\nConnection: keep-alive\r\n\r\n";
    assert(http_parse_request(raw, strlen(raw), &req) == 0);
    assert(strcmp(req.method, "GET") == 0);
    assert(strcmp(req.path, "/index.html") == 0);
    assert(strcmp(req.host, "alice.eezydep.org") == 0);
    assert(req.content_length == 10);
    assert(req.keep_alive == 1);
    printf("PASS: test_full_get\n");
}

void test_defaults(){
    const char *raw = "GET /index.html HTTP/1.1\r\nHost: alice.eezydep.org\r\n\r\n";
    http_request_t req;
    assert(http_parse_request(raw, strlen(raw), &req) == 0);
    assert(strcmp(req.method, "GET") == 0);
    assert(strcmp(req.path, "/index.html") == 0);
    assert(strcmp(req.host, "alice.eezydep.org") == 0);
    assert(req.content_length == -1);
    assert(req.keep_alive == 0);
    printf("PASS: test_defaults\n");
}
void test_no_host() {
    const char *raw = "GET /index.html HTTP/1.1\r\n\r\n";
    http_request_t req;
    assert(http_parse_request(raw, strlen(raw), &req) == -1);
    printf("PASS: test_no_host\n");
}

void test_empty() {
    const char *raw = "";
    http_request_t req;
    assert(http_parse_request(raw, strlen(raw), &req) == -1);
    printf("PASS: test_empty\n");
}

int main(void){
    test_basic_get();
    test_full_get();
    test_defaults();
    test_no_host();
    test_empty();
}
