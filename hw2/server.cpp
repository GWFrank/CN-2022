#include"server.h"

#include <opencv2/opencv.hpp>

#include<stdio.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<sys/ioctl.h>
#include<net/if.h>
#include<unistd.h> 
#include<string.h>
#include<stdlib.h>
#include<pthread.h>
#include<vector>

#define BUFF_SIZE 1024
#define PORT 8787
#define ERR_EXIT(a){ perror(a); exit(1); }
#define LL long long

void* serve_client(void* arg) {
    struct clientInfo cinfo = *(struct clientInfo *)arg;
    char buffer[2 * BUFF_SIZE] = {};
    int write_byte;
    
    // cv::Mat server_img;
    // cv::VideoCapture cap("./video.mpg");
    // int width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    // int height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    // printf("[info] Video resolution: %d, %d\n", width, height);

    // server_img = cv::Mat::zeros(height, width, CV_8UC3);
    // if (!server_img.isContinuous()) {
    //     server_img = server_img.clone();
    // }

    // while (1) {
    //     cap >> server_img;
    //     // int imgSize = 
    // }

    char message[BUFF_SIZE] = "Hello World from Server";
    sprintf(buffer, "%s\n", message);
    if((write_byte = send(cinfo.sock_fd, buffer, strlen(buffer), 0)) < 0){
        ERR_EXIT("write failed\n");
    }
    printf("[info] thread %lu sent %d to fd %d\n", cinfo.tid, write_byte, cinfo.sock_fd);


    close(cinfo.sock_fd);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]){
    int server_sockfd, client_sockfd; //, write_byte;
    struct sockaddr_in server_addr, client_addr;
    int client_addr_len = sizeof(client_addr);
    // char buffer[2 * BUFF_SIZE] = {};
    // char message[BUFF_SIZE] = "Hello World from Server";

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

    std::vector<clientInfo> clients(0);
    while (1) {
        // Accept the client and get client file descriptor
        if((client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_addr, (socklen_t*)&client_addr_len)) < 0){
            ERR_EXIT("accept failed\n");
        }
        printf("[info] new connection accepted\n");
        struct clientInfo cinfo;
        cinfo.sock_fd = client_sockfd;
        clients.push_back(cinfo);
        int l_idx = clients.size()-1;
        pthread_create(&clients[l_idx].tid, NULL, &serve_client, (void*)&clients[l_idx]);
    }
    
    // sprintf(buffer, "%s\n", message);
    // if((write_byte = send(client_sockfd, buffer, strlen(buffer), 0)) < 0){
    //     ERR_EXIT("write failed\n");
    // }
    // close(client_sockfd);

    for (auto &cl:clients) {
        pthread_join(cl.tid, NULL);
    }
    close(server_sockfd);
    return 0;
}
