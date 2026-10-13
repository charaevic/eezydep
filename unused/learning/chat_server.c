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
#include <sys/epoll.h>

#define PORT "3490"
#define BACKLOG 10
#define MAXDATASIZE 100
#define MAXCONNECTIONS 59
#define MAXEVENTS 119

int main(void){
    int clients[MAXCONNECTIONS];
    int client_count = 0;

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
    int epoll_fd = epoll_create1(0); //Create new epoll (interface list)
    bind(server_fd, res->ai_addr, res->ai_addrlen);// bind socket
    listen(server_fd, BACKLOG); //bind listening socket
    struct epoll_event ee;
    ee.events = EPOLLIN;
    ee.data.fd = server_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ee);
    //setup and added socket to epoll 

    //main event loop where either we connect new client and add to epoll or we echo message to all connected clients in epoll?

    while(1){
        struct epoll_event events[MAXEVENTS];
        int n = epoll_wait(epoll_fd, events, MAXEVENTS, -1);
        for(int i = 0; i <n; i++){
            //case fd is listening socket
            if (events[i].data.fd == server_fd){
                struct sockaddr_storage client_addr;
                socklen_t addr_size;
                int new_conn = accept(server_fd, (struct sockaddr* ) &client_addr, &addr_size);
                struct epoll_event new_event;
                new_event.events = EPOLLIN;
                new_event.data.fd = new_conn;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_conn, &new_event);
                clients[client_count++] = new_conn;
            }
            //case 2 it is one of the connected clients TODO
            else{
                int sender_fd = events[i].data.fd;
                char buf[MAXDATASIZE];
                
                if((numbytes = recv(sender_fd, buf, MAXDATASIZE-1, 0))>0){
                    for(int j = 0; j < client_count; j++){
                        if(clients[j] != sender_fd){
                            send(clients[j], buf, numbytes, 0);
                        }
                    }
                } else{ //disconnect routine
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sender_fd, NULL);
                    close(sender_fd);
                    for (int k = 0; k < client_count; k++) {
                        if (clients[k] == sender_fd) {
                            clients[k] = clients[--client_count];
                            break;
                        }
                    }
                }
            }
        }


    }


}