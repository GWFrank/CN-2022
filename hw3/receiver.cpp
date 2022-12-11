#include "rdt.hpp"

#include <opencv2/opencv.hpp>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

void unpack_uint64(uint64 &dst, const rdt::SEGMENT *src) {
    dst = 0;
    for (int i = 0; i < 8; ++i) {
        // fprintf(stderr, "data[%d] = %x\n", i, (unsigned char)src->data[i]);
        dst |= (unsigned char)(src->data[i]) << 8 * i;
    }
    // fprintf(stderr, "\n");
}

void unpack_frame(cv::Mat &dst, const rdt::SEGMENT *src, uint64 &idx) {
    memcpy(dst.data + idx, src->data, src->header.length);
    idx += src->header.length;
}

// Return whether to flush buffer
bool process_received_segment(
    const rdt::SEGMENT *received_segment, rdt::SEGMENT *ack_segment,
    int &next_seq_number, rdt::SEGMENT receive_buffer[rdt::Default_Buffer_Size],
    int &next_empty_idx, bool &streaming) {
    int seq_number = received_segment->header.seqNumber;
    // Check segment order
    if (seq_number != next_seq_number) {
        rdt::initSegment(ack_segment, next_seq_number - 1);
        rdt::setACK(ack_segment, next_seq_number - 1, false);
        if (!received_segment->header.fin) {
            printf("drop\tdata\t#%d\t(out of order)\n", seq_number);
        } else {
            printf("drop\tfin\t\t(out of order)\n");
        }
        return false;
    }

    // Check data integrity
    if (!rdt::validChecksum(received_segment)) {
        rdt::initSegment(ack_segment, next_seq_number - 1);
        rdt::setACK(ack_segment, next_seq_number - 1, false);
        printf("drop\tdata\t#%d\t(corrupted)\n", seq_number);
        return false;
    }

    // Check buffer overflow
    if (next_empty_idx >= rdt::Default_Buffer_Size) {
        rdt::initSegment(ack_segment, next_seq_number - 1);
        rdt::setACK(ack_segment, next_seq_number - 1, false);
        printf("drop\tdata\t#%d\t(buffer overflow)\n", seq_number);
        return true;
    }

    // Check fin
    if (received_segment->header.fin) {
        rdt::initSegment(ack_segment, next_seq_number);
        rdt::setACK(ack_segment, next_seq_number, true);
        printf("recv\tfin\n");
        streaming = false;
        return true;
    }

    // Put segment in buffer
    receive_buffer[next_empty_idx] = *received_segment;
    next_empty_idx++;
    next_seq_number++;
    rdt::initSegment(ack_segment, next_seq_number - 1);
    rdt::setACK(ack_segment, next_seq_number - 1, false);
    printf("recv\tdata\t#%d\n", seq_number);
    return false;
}

void send_ack(const rdt::SEGMENT *ack_segment, const int receiver_socket,
              const struct sockaddr_in &agent_addr, socklen_t agent_addr_size) {
    // rdt::printHeader(ack_segment);
    int sent_bytes = sendto(receiver_socket, ack_segment, sizeof(rdt::SEGMENT),
                            0, (struct sockaddr *)&agent_addr, agent_addr_size);
    if (sent_bytes < 0) {
        perror("sendto() error...");
        exit(1);
    } else {
        if (!ack_segment->header.fin) {
            printf("send\tack\t#%d\n", ack_segment->header.ackNumber);
        } else {
            printf("send\tfinack\n");
        }
        // fprintf(stderr, "[INFO] sendto() sent %d bytes\n", sent_bytes);
    }
}

void flush_buffer(rdt::SEGMENT receive_buffer[rdt::Default_Buffer_Size],
                  int &next_empty_idx, cv::Mat &frame_container,
                  uint64 &frame_data_idx) {
    printf("flush\n");
    uint64 width = 0, height = 0;
    for (int i = 0; i < next_empty_idx; ++i) {
        switch (receive_buffer[i].header.seqNumber) {
            case 1:
                unpack_uint64(width, &receive_buffer[i]);
                break;
            case 2:
                unpack_uint64(height, &receive_buffer[i]);
                // fprintf(stderr, "[INFO] resolution=%lux%lu\n", width, height);
                frame_container = cv::Mat::zeros(height, width, CV_8UC3);
                if (!frame_container.isContinuous()) {
                    frame_container = frame_container.clone();
                }
                break;
            default:
                unpack_frame(frame_container, &receive_buffer[i],
                             frame_data_idx);
                if (frame_data_idx ==
                    frame_container.total() * frame_container.elemSize()) {
                    frame_data_idx = 0;
                    cv::imshow("Video", frame_container);
                    cv::waitKey(500);
                }
                break;
        }
    }
    next_empty_idx = 0;
    // // Testing with  string transmission
    // for (int i = 0; i < next_empty_idx; ++i) {
    //     printf("data #%d: %s\n", receive_buffer[i].header.seqNumber,
    //            receive_buffer[i].data);
    // }
    // next_empty_idx = 0;
}

int main(int argc, char *argv[]) {
    int receiver_port, agent_port;
    char receiver_ip[32], agent_ip[32];
    int receiver_socket;
    struct sockaddr_in receiver_addr, agent_addr;
    socklen_t receiver_addr_size, agent_addr_size;

    // Parse args
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <receiver port> <agent IP>:<agent port>\n",
                argv[0]);
        exit(1);
    }
    sscanf(argv[1], "%d", &receiver_port);
    snprintf(receiver_ip, 32, "127.0.0.1");
    sscanf(argv[2], "%[^:]:%d", agent_ip, &agent_port);

    fprintf(stderr, "receiver port = %d\n", receiver_port);
    fprintf(stderr, "agent ip = %s, agent port = %d\n", agent_ip, agent_port);

    // Prepare sockets
    receiver_socket = socket(PF_INET, SOCK_DGRAM, 0);
    if (receiver_socket < 0) {
        perror("socket error...");
        exit(1);
    }
    rdt::setup_sockaddr(receiver_addr, receiver_addr_size, receiver_ip,
                        receiver_port);
    if (bind(receiver_socket, (struct sockaddr *)&receiver_addr,
             sizeof(receiver_addr)) < 0) {
        perror("bind error...");
        exit(1);
    }

    rdt::setup_sockaddr(agent_addr, agent_addr_size, agent_ip, agent_port);

    // Prepare streaming
    int next_seq_number = 1;

    rdt::SEGMENT receive_buffer[rdt::Default_Buffer_Size];
    int next_empty_idx = 0;
    rdt::SEGMENT ack_segment;

    rdt::SEGMENT received_segment;
    struct sockaddr_in source_addr;
    socklen_t source_addr_size = sizeof(source_addr);
    char source_ip[1000];
    int source_port;

    cv::Mat frame_container;
    uint64 frame_data_idx = 0;

    bool streaming = true;

    // Main transmit loop
    while (streaming) {
        // Receive segment
        memset(&received_segment, 0, sizeof(rdt::SEGMENT));
        int recv_ret = recvfrom(
            receiver_socket, &received_segment, sizeof(received_segment), 0,
            (struct sockaddr *)&source_addr, &source_addr_size);

        if (recv_ret < 0) {  // Error
            perror("recvfrom() error...");
            exit(1);
        }

        // fprintf(stderr, "[INFO] received %d bytes\n", recv_ret);

        // Throw away packets not from agent
        inet_ntop(AF_INET, &source_addr.sin_addr.s_addr, source_ip,
                  sizeof(source_ip));
        source_port = ntohs(source_addr.sin_port);
        // fprintf(stderr, "[INFO] source ip: %s, source port: %d, addr_size:
        // %d\n", source_ip, source_port, source_addr_size);
        if (strcmp(source_ip, agent_ip) != 0 || source_port != agent_port) {
            // fprintf(stderr, "[INFO] received packets not from agent\n");
            continue;
        }

        // Should not be finack nor ack
        if (received_segment.header.ack) {
            fprintf(stderr, "Received ACK, exiting...\n");
            exit(1);
        }

        bool buffer_full = process_received_segment(
            &received_segment, &ack_segment, next_seq_number, receive_buffer,
            next_empty_idx, streaming);
        send_ack(&ack_segment, receiver_socket, agent_addr, agent_addr_size);

        if (buffer_full) {
            flush_buffer(receive_buffer, next_empty_idx, frame_container,
                         frame_data_idx);
        }
    }

    cv::destroyAllWindows();
    close(receiver_socket);

    return 0;
}