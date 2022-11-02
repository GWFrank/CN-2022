#include "server.h"
#include "pkt_util.hpp"
#include "cmd_util.hpp"

#include <opencv2/opencv.hpp>

#include <vector>
#include <string>
#include <set>
#include <iostream>

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

void* srv_interact(void* arg) {
    cmu::clientInfo* cinfo_p = (cmu::clientInfo *)arg;
    char in_buf[MAX_BUF_SIZE] = "";
    // char out_buf[MAX_BUF_SIZE] = "";
    char path_buf[MAX_BUF_SIZE] = "";
    // Login
    if (pku::recv_str(cinfo_p->sock_fd, in_buf)) {
        goto srv_interact_exit;
    }
    cinfo_p->username = in_buf;
    printf("Accept a new connection on socket [%d]. Login as %s\n", cinfo_p->sock_fd, cinfo_p->username.c_str());
    // Check ./server_dir and ./server_dir/<username>
    cmu::check_and_mkdir(cmu::SRVDIR);
    snprintf(path_buf, MAX_BUF_SIZE, "%s/%s", cmu::SRVDIR, cinfo_p->username.c_str());
    cmu::check_and_mkdir(path_buf);
    // Shell loop
    // fprintf(stderr, "[info] Entering shell loop\n");
    while (1) {
        // Receive command
        if (pku::recv_str(cinfo_p->sock_fd, in_buf)) {
            goto srv_interact_exit;
        }
        std::string cmd(in_buf);
        // Check permission
        int ret = cmu::check_permission_srv(cinfo_p, cmd);
        if (ret == 1) { // Allowed
            // Check command validity and run
            if (cmu::ALL_CMDS.count(cmd)) {
                if (cmu::srv_cmd_func.find(cmd)->second(cinfo_p)) {
                    goto srv_interact_exit;
                }
            } else {
                // fprintf(stderr, "[info] Client's command not found.\n");
            }
        } else if (ret == 0) { // Banned
            continue;
        } else if (ret == -1) { // Errored
            goto srv_interact_exit;
        } else { // Weird cases
            ERR_EXIT("Unexpected return from permission checking");
        }
    }
    
srv_interact_exit:
    close(cinfo_p->sock_fd);
    // free(arg);
    delete (cmu::clientInfo*)arg;
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

    // fprintf(stderr, "[info] Server listening on port %d\n", PORT);

    // Listen for clients loop
    std::set<std::string> blocklist;
    pthread_mutex_t blocklist_lock;
    pthread_mutex_init(&blocklist_lock, NULL);
    while (1) {
        // Accept the client and get client file descriptor
        if((client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_addr, (socklen_t*)&client_addr_len)) < 0){
            ERR_EXIT("accept failed\n");
        }
        // fprintf(stderr, "[info] new connection accepted\n");
        cmu::clientInfo* new_cl_p = new cmu::clientInfo(client_sockfd, &blocklist, &blocklist_lock);
        // new_cl_p->sock_fd = client_sockfd;
        // new_cl_p->srv_blocklist_p = &blocklist;
        // new_cl_p->lock_p = &blocklist_lock;
        pthread_create(&(new_cl_p->tid), NULL, &srv_interact, (void*)new_cl_p);
        pthread_detach(new_cl_p->tid);
    }

    close(server_sockfd);
    return 0;
}
