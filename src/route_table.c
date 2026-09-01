#include "route_table.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
route_profile* route_lookup(route_profile* route_table, size_t len, char* hostname){//lookup a route 
    printf("Table size: %zu\n", len);
    if (route_table != NULL){
        route_profile* iterator = route_table;
        for (int i = 0; i < len; i++) {
            if (strcmp(hostname, iterator->host) == 0){
                return iterator;
            }
            iterator++;
        }
        return NULL;
    }
    return NULL;
}
int route_load(const char *filepath, route_profile *table, size_t max_routes){
    int count = 0;
    FILE* file = fopen(filepath, "r");
    if (file == NULL){return -1;}
    char buffer[1024];
    while((fgets(buffer, sizeof(buffer), file) != NULL) && count < max_routes){
        char hostname[256];
        char addr[64];
        int port;
        if (sscanf(buffer, "%s %[^:]:%d", hostname, addr, &port) != 3){
            return -1;
        }
        //we now have them split, lets just create a new route profile 
        strncpy(table[count].host, hostname, sizeof(table[count].host) - 1);
        strncpy(table[count].backend_addr, addr, sizeof(table[count].backend_addr) - 1);
        table[count].backend_port = port;
        count++;

    }
    fclose(file);
    return count;

}



