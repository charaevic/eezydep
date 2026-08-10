#ifndef ROUTE_TABLE
#define ROUTE_TABLE
#include <stddef.h>
typedef struct{
    char host[256];
    char backend_addr[64];
    int backend_port;
} route_profile;
route_profile* route_lookup(route_profile* route_table, size_t len, char* hostname);
int route_load(const char *filepath, route_profile *table, size_t max_routes);
#endif