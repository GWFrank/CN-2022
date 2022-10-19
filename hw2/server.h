#include <pthread.h>

struct clientInfo {
    pthread_t tid;
    int sock_fd;
    clientInfo() : tid(0), sock_fd(0) {}
    clientInfo(int _sock_fd) : tid(0), sock_fd(_sock_fd) {}
};