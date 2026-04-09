#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "3490"
#define BACKLOG 10
#define MAXDATASIZE 100
int main(void){
    struct addrinfo hints, *res;
    struct sockaddr_storage client_addr;
    socklen_t addr_size;
    int numbytes, numsent;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    getaddrinfo(NULL, PORT, &hints, &res);

    int server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    bind(server_fd, res->ai_addr, res->ai_addrlen);
    listen(server_fd, BACKLOG);

    addr_size = sizeof client_addr;
    int connected_fd = accept(server_fd, (struct sockaddr* ) &client_addr, &addr_size);
    char buf[MAXDATASIZE];

    while((numbytes = recv(connected_fd, buf, MAXDATASIZE-1, 0)) > 0){
        printf("Received data, echoing back\n");
        fflush(stdout);
        numsent = send(connected_fd, buf, numbytes, 0);
        
    }
    printf("Loop exit (conn closed/error?)\n");
    freeaddrinfo(res);
    close(connected_fd);
    close(server_fd);

    return 0;

}
