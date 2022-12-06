#include "rdt.hpp"

#include <opencv2/opencv.hpp>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

int main(int argc, char *argv[]) {
    int recv_port, agent_port;
    char recv_ip[32], agent_ip[32], filename[PATH_MAX];

    // From agent.cpp
    if (argc != 3) {
        fprintf(stderr,
                "Usage: %s <receiver port> <agent IP>:<agent port>\n",
                argv[0]);
        exit(1);
    } else {
        sscanf(argv[1], "%d", &recv_port);
        snprintf(recv_ip, 32, "127.0.0.1");
        sscanf(argv[2], "%[^:]:%d", agent_ip, &agent_port);
    }

    return 0;
}