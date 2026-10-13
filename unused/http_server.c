

#define PORT "3490"
#define BACKLOG 10
#define MAXDATASIZE 100
#define MAXCONNECTIONS 59
#define MAXEVENTS 119
#define BASE "./www"
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
                socklen_t addr_size = sizeof(client_addr);
                int new_conn = accept(server_fd, (struct sockaddr* ) &client_addr, &addr_size);
                struct epoll_event new_event;
                new_event.events = EPOLLIN;
                new_event.data.fd = new_conn;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_conn, &new_event);
                clients[client_count++] = new_conn;
            }
            //case 2 we have an HTTP req take it and parse it

            else{
                int sender_fd = events[i].data.fd;
                char buf[MAXDATASIZE];
                //received request
                if((numbytes = recv(sender_fd, buf, MAXDATASIZE-1, 0))>0){
                    printf("Received bytes: \n");
                    buf[numbytes] = '\0';
                    char* token1 = strtok(buf, " ");
                    char* token2 = strtok(NULL, " ");
                    const char* basefolder = BASE;
                    char fullPath[PATH_MAX];
                    
                    snprintf(fullPath, sizeof(fullPath), "%s%s", basefolder, token2);
                    
                    FILE* file = fopen(fullPath, "r");
                    //TODO return HTTP 404 Not Found
                    if (file != NULL){
                        struct stat st;
                        stat(fullPath, &st);
                        printf("\nFile requested found\n");
                        char header[1024];
                        snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n", st.st_size);
                        send(sender_fd, header, strlen(header), 0);
                        char buffer[1024];
                        size_t bytes_read;
                        while((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0){
                            printf("%.*s", (int)bytes_read, buffer);
                            send(sender_fd, buffer, bytes_read, 0);
                        }
                        fclose(file);
                        printf("full path: %s\n", fullPath);
                    } else{
                        char header[1024];
                        char* body = "404 Not Found";
                        snprintf(header, sizeof(header), "HTTP/1.1 404 Not Found\r\nContent-Length: %zu\r\n\r\n", strlen(body));
                        send(sender_fd, header, strlen(header), 0);
                        send(sender_fd, body, strlen(body), 0);
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