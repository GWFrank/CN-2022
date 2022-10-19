#include "server.h"
#include "pkt_util.hpp"

#include <opencv2/opencv.hpp>

#include <vector>

#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <csignal>

const char SHELL_SYMBOL[] = "$ ";

void* srv_interact(void* arg) {
    clientInfo cinfo = *(clientInfo *)arg; // Includes tid for debugging
    char out_buf[MAX_BUF_SIZE] = "";
    char in_buf[MAX_BUF_SIZE] = "";
    // Login
    recv_str(cinfo.sock_fd, in_buf);
    fprintf(stderr, "[login] User '%s' logged in\n", in_buf);
    // Test video
    // cv::VideoCapture cap("./video.mpg");
    // if (send_video(cinfo.sock_fd, cap) == 1) {
    if (send_video(cinfo.sock_fd, "./dQw4w.mpg") == 1) {
        goto src_interact_exit;
    }
    // Shell loop
    fprintf(stderr, "[info] Entering shell loop\n");
    while (1) {
        snprintf(out_buf, MAX_BUF_SIZE, "%s", SHELL_SYMBOL);
        if (send_str(cinfo.sock_fd, out_buf) == 1) {
            goto src_interact_exit;
        }
        if (recv_str(cinfo.sock_fd, in_buf) == 1) {
            goto src_interact_exit;
        }
        printf("Client sent '%s'\n", in_buf);
    }
    
src_interact_exit:
    close(cinfo.sock_fd);
    free(arg);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]){
    int PORT;
    // Handle arguments
    if (argc < 2) {
        fprintf(stderr, "Usage: ./server <port>\n");
        exit(1);
    }
    if ((PORT = atoi(argv[1])) == 0) {
        fprintf(stderr, "Invalid port number given\n");
        exit(1);
    }

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

    // Allow reuse socket to avoid "address already in used" when restarting
    int sock_reuse_addr = 1;
    setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &sock_reuse_addr, sizeof(int));
    
    // Bind the server file descriptor to the server address
    if(bind(server_sockfd, (struct sockaddr *)&server_addr , sizeof(server_addr)) < 0){
        ERR_EXIT("bind failed\n");
    }
        
    // Listen on the server file descriptor
    if(listen(server_sockfd , 3) < 0){
        ERR_EXIT("listen failed\n");
    }

    fprintf(stderr, "[info] Server listening on port %d\n", PORT);

    // Listen for clients loop
    // std::vector<clientInfo> clients(0);
    while (1) {
        // Accept the client and get client file descriptor
        if((client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_addr, (socklen_t*)&client_addr_len)) < 0){
            ERR_EXIT("accept failed\n");
        }
        fprintf(stderr, "[info] new connection accepted\n");
        // clients.push_back(clientInfo(client_sockfd));
        // clientInfo* new_cl_p = &clients[clients.size()-1];
        clientInfo* new_cl_p = (clientInfo*)malloc(sizeof(clientInfo));
        *new_cl_p = clientInfo(client_sockfd);
        pthread_create(&(new_cl_p->tid), NULL, &srv_interact, (void*)new_cl_p);
        pthread_detach(new_cl_p->tid);
    }

    close(server_sockfd);
    return 0;
}
