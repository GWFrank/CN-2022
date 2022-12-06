#include "rdt.hpp"

#include <opencv2/opencv.hpp>

#include <deque>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>

void pack_int64(const int64_t src, char* dst) {
    for (int i=0; i<64/8; ++i) {
        dst[i] = src >> 8*i;
    }
}

int main(int argc, char *argv[]) {
    int sender_port, agent_port;
    char sender_ip[32], agent_ip[32], filename[PATH_MAX];
    int sender_socket;
    struct sockaddr_in sender_addr, agent_addr;
    socklen_t sender_addr_size, agent_addr_size;
    
    // Parse args (from agent.cpp)
    if (argc != 4) {
        fprintf(stderr,
                "Usage: %s <sender port> <agent IP>:<agent port> <.mpg filename>\n",
                argv[0]);
        exit(1);
    }

    sscanf(argv[1], "%d", &sender_port);
    snprintf(sender_ip, 32, "127.0.0.1");
    sscanf(argv[2], "%[^:]:%d", agent_ip, &agent_port);
    strncpy(filename, argv[3], PATH_MAX);

    // Prepare sockets (from agent.cpp)
    sender_socket = socket(PF_INET, SOCK_DGRAM, 0);
    sender_addr.sin_family = AF_INET;
    sender_addr.sin_port = htons(sender_port);
    sender_addr.sin_addr.s_addr = inet_addr(sender_ip);
    memset(sender_addr.sin_zero, 0, sizeof(sender_addr.sin_zero));
    sender_addr_size = sizeof(sender_addr);

    bind(sender_socket, (struct sockaddr*)&sender_addr, sizeof(sender_addr));

    agent_addr.sin_family = AF_INET;
    agent_addr.sin_port = htons(agent_port);
    agent_addr.sin_addr.s_addr = inet_addr(agent_ip);
    memset(agent_addr.sin_zero, 0, sizeof(agent_addr.sin_zero));
    agent_addr_size = sizeof(agent_addr);

    std::deque<rdt::SEGMENT> segment_queue;
    int current_window = rdt::Default_Window_Size;
    int SSthreshold = rdt::Default_SSthreshold;
    int ack_count = 0;
    int next_seq_number = 1;
    struct timeval ack_wait;

    fd_set socks, readsocks;
    FD_ZERO(&socks);
    FD_SET(sender_socket, &socks);

    rdt::SEGMENT received_segment;
    struct sockaddr_in source_addr;
    socklen_t source_addr_size;
    char source_ip[32];
    int source_port;

    bool streaming = true;

    // Main transmit loop
    while (streaming) {
        // Fill queue to >= Default_Window_Size
        while (segment_queue.size() < current_window) {
            rdt::SEGMENT tmp_seg;
            rdt::initHeader(&tmp_seg.header, next_seq_number, 0);
            next_seq_number++;
            // Test string transmission
            printf("Enter message > ");
            scanf("%s", tmp_seg.data);
            tmp_seg.header.length = strlen(tmp_seg.data);
            rdt::setChecksum(&tmp_seg);
            segment_queue.push_back(tmp_seg);
        }
        // Transmit a window of segments
        for (int i=0; i<current_window; ++i) {
            sendto(sender_socket, &segment_queue[i], sizeof(segment_queue[i]), 0, (struct sockaddr*)&agent_addr, agent_addr_size);
            printf("send\tdata\t#%d,\twinSize = %d\n", segment_queue[i].header.seqNumber, current_window);
        }

        // Receive ACKs
        bool receiving = true;
        ack_wait.tv_sec = rdt::Default_Timeout_Seconds;
        ack_wait.tv_usec = 0;
        memcpy(&readsocks, &socks, sizeof(fd_set));

        while (receiving) {
            // Implement timeout with select.
            // Ref: http://alumni.cs.ucr.edu/~jiayu/network/lab8.htm
            
            if (select(1, &readsocks, NULL, NULL, &ack_wait) < 0) { // Error
                perror("select() error...");
            }

            if (FD_ISSET(sender_socket, &readsocks)) { // Segments available
                memset(&received_segment, 0, sizeof(received_segment));
                
                int recv_ret = recvfrom(sender_socket, &received_segment, sizeof(received_segment), 0, (struct sockaddr*)&source_addr, &source_addr_size);
                if (recv_ret < 0) { // Error
                    perror("recvfrom() error...");
                }
                // Throw away packets not from agent
                inet_ntop(AF_INET, &source_addr.sin_addr.s_addr, source_ip, sizeof(source_ip));
                source_port = ntohs(source_addr.sin_port);
                if (strcmp(source_ip, agent_ip) != 0 || source_port != agent_port) {
                    continue;
                }
                // Should be either finack or ack
                if (!received_segment.header.ack) {
                    fprintf(stderr, "Received non-ACK, exiting...\n");
                    exit(1);
                }
                // Should be uncorrupted
                if (!validChecksum(&received_segment)) {
                    fprintf(stderr, "ACKs shouldn't be corrupted, exiting...\n");
                    exit(1);
                }
                
                if (received_segment.header.fin) { // Finack
                    receiving = false;
                    streaming = false;
                    continue;
                } else { // Normal ack
                    printf("recv\tack\t#%d\n");
                    int ack_num = received_segment.header.ackNumber;
                    while (!segment_queue.empty() && ack_num <= segment_queue[0].header.seqNumber) {
                        segment_queue.pop_front();
                        ack_count++;
                    }
                    if (ack_count == current_window) {
                        // TODO: implement congestion control
                        receiving = false;
                    }
                }
                    
                
            } else { // Timeout
                // TODO: implement congestion control
                continue;
            }
        }

    }

    return 0;
}