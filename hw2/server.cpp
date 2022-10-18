#include "server.h"
#include "pkt_util.h"

#include <opencv2/opencv.hpp>

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h> 
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <pthread.h>
#include <vector>

#define PORT 7654

const char SHELL_SYMBOL[] = "$ ";

void* srv_interact(void* arg) {
    clientInfo cinfo = *(clientInfo *)arg; // Includes tid for debugging
    char out_buf[MAX_BUF_SIZE] = "";
    char in_buf[MAX_BUF_SIZE] = "";
    // snprintf(out_buf, MAX_BUF_SIZE, "Welcome to the server!\n");
    // send_str(cinfo.sock_fd, out_buf);

    // Shell loop
    while (1) {
        snprintf(out_buf, MAX_BUF_SIZE, SHELL_SYMBOL);
        if (send_str(cinfo.sock_fd, out_buf) == 1) {
            break;
        }
        if (recv_str(cinfo.sock_fd, in_buf) == 1) {
            break;
        }
        printf("Client sent '%s'\n", in_buf);
    }
    
    close(cinfo.sock_fd);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]){

    int server_sockfd, client_sockfd; 
    struct sockaddr_in server_addr, client_addr;
    int client_addr_len = sizeof(client_addr);

    // Get socket file descriptor
    if((server_sockfd = socket(AF_INET , SOCK_STREAM , 0)) < 0){
        ERR_EXIT("socket failed\n")
    }

    // Set server address information
    bzero(&server_addr, sizeof(server_addr)); // erase the data
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);
    
    // Bind the server file descriptor to the server address
    if(bind(server_sockfd, (struct sockaddr *)&server_addr , sizeof(server_addr)) < 0){
        ERR_EXIT("bind failed\n");
    }
        
    // Listen on the server file descriptor
    if(listen(server_sockfd , 3) < 0){
        ERR_EXIT("listen failed\n");
    }

    // Get client loop
    std::vector<clientInfo> clients(0);
    while (1) {
        // Accept the client and get client file descriptor
        if((client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_addr, (socklen_t*)&client_addr_len)) < 0){
            ERR_EXIT("accept failed\n");
        }
        fprintf(stderr, "[info] new connection accepted\n");
        clients.push_back(clientInfo(client_sockfd));
        clientInfo* new_cl_p = &clients[clients.size()-1];
        pthread_create(&(new_cl_p->tid), NULL, &srv_interact, (void*)new_cl_p);
        pthread_detach(new_cl_p->tid);
    }

    close(server_sockfd);
    return 0;
}
