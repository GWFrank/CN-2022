#include "pkt_util.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define PORT 7654

void cli_interact(int sock_fd) {
    // Receive message from server
    char in_buf[MAX_BUF_SIZE] = "";
    char out_buf[MAX_BUF_SIZE] = "";
    // recv_str(sock_fd, in_buf);
    // Shell loop
    while (1) {
        recv_str(sock_fd, in_buf);
        printf("%s", in_buf);
        fgets(out_buf, MAX_BUF_SIZE-1, stdin); // fgets() reads spaces
        send_str(sock_fd, out_buf);
    }
}

int main(int argc, char *argv[]) {
    int sock_fd;
    struct sockaddr_in addr;

    // Get socket file descriptor
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        ERR_EXIT("socket failed\n");
    }

    // Set server address
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(PORT);

    // Connect to the server
    if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        ERR_EXIT("connect failed\n");
    }

    cli_interact(sock_fd);

    close(sock_fd);
    return 0;
}
