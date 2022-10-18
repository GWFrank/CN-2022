#include<pthread.h>

struct clientInfo {
    pthread_t tid;
    int sock_fd;
};