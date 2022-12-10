#include "rdt.hpp"

#include <zlib.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>

namespace rdt {
void initSegment(SEGMENT* segment, int seq_number) {
    segment->header.length = 0;
    segment->header.seqNumber = seq_number;
    segment->header.ackNumber = 0;
    segment->header.fin = 0;
    segment->header.syn = 0;
    segment->header.ack = 0;
    segment->header.checksum = 0;
    memset(segment->data, 0, sizeof(char) * SEG_DATA_LEN);
}

void setACK(SEGMENT* segment, int ack_number, bool ack_fin) {
    segment->header.ack = 1;
    segment->header.ackNumber = ack_number;
    if (ack_fin) segment->header.fin = 1;
}

void setChecksum(SEGMENT* segment) {
    segment->header.checksum =
        crc32(0L, (const Bytef*)segment->data, segment->header.length);
}

int validChecksum(const SEGMENT* segment) {
    unsigned long calc_checksum;
    calc_checksum =
        crc32(0L, (const Bytef*)segment->data, segment->header.length);
    return calc_checksum == segment->header.checksum;
}

void printHeader(const SEGMENT* segment) {
    fprintf(stderr, "[RDT INFO] header.length = %d\n", segment->header.length);
    fprintf(stderr, "[RDT INFO] header.seqNumber = %d\n",
            segment->header.seqNumber);
    fprintf(stderr, "[RDT INFO] header.ackNumber = %d\n",
            segment->header.ackNumber);
    fprintf(stderr, "[RDT INFO] header.fin = %d\n", segment->header.fin);
    fprintf(stderr, "[RDT INFO] header.syn = %d\n", segment->header.syn);
    fprintf(stderr, "[RDT INFO] header.ack = %d\n", segment->header.ack);
    fprintf(stderr, "[RDT INFO] header.checksum = %lu\n",
            segment->header.checksum);
}

void setup_socket_timeout(int socket_fd) {
    struct timeval timeout;
    timeout.tv_sec =
        rdt::Default_Timeout_Seconds;  // Set timeout value to 5 seconds
    timeout.tv_usec = 0;
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
}

void unblock_socket(int socket_fd) {
    int socket_flags;
    if ((socket_flags = fcntl(socket_fd, F_GETFL)) < 0) {
        perror("Get socket flag error...");
        exit(1);
    }
    if (fcntl(socket_fd, F_SETFL, socket_flags | O_NONBLOCK) < 0) {
        perror("Set non-blocking flag error...");
        exit(1);
    }
}

void setup_sockaddr(struct sockaddr_in& new_sockaddr, socklen_t& addr_len,
                    char* ip, int port) {
    new_sockaddr.sin_family = AF_INET;
    new_sockaddr.sin_port = htons(port);
    new_sockaddr.sin_addr.s_addr = inet_addr(ip);
    memset(new_sockaddr.sin_zero, 0, sizeof(new_sockaddr.sin_zero));
    addr_len = sizeof(new_sockaddr);
}

}  // namespace rdt
