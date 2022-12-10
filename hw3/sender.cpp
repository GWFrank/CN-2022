#include "rdt.hpp"

#include <opencv2/opencv.hpp>

#include <algorithm>
#include <deque>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <cassert>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

void pack_uint64(const uint64 src, rdt::SEGMENT *dst) {
    for (int i = 0; i < 64 / 8; ++i) {
        dst->data[i] = src >> 8 * i;
    }
}

void pack_frame(const cv::Mat &src, rdt::SEGMENT *dst, uint64 &idx,
                uint64 size) {
    memcpy(dst->data, src.data + idx, size);
    dst->header.length = size;
}

void enqueue_frame(std::deque<rdt::SEGMENT> &segment_queue,
                   int &next_seq_number, cv::VideoCapture &cap,
                   cv::Mat &frame_container) {
    cap >> frame_container;
    uint64 frame_size = frame_container.total() * frame_container.elemSize();
    rdt::SEGMENT tmp_seg;

    if (frame_size == 0) {  // End of video
        rdt::initSegment(&tmp_seg, next_seq_number);
        tmp_seg.header.fin = 1;
        fprintf(stderr, "[INFO] Reached end of video\n");
        strcpy(tmp_seg.data, "Goodbye!");
        tmp_seg.header.length = strlen(tmp_seg.data);
        rdt::setChecksum(&tmp_seg);
        segment_queue.push_back(tmp_seg);
        return;
    }

    for (uint64 i = 0; i < frame_size; i += 1000) {
        rdt::initSegment(&tmp_seg, next_seq_number);
        uint64 size_left = std::min((uint64)1000, frame_size - i);
        pack_frame(frame_container, &tmp_seg, i, size_left);
        rdt::setChecksum(&tmp_seg);
        segment_queue.push_back(tmp_seg);
        next_seq_number++;
    }
}

void enqueue_metadata(std::deque<rdt::SEGMENT> &segment_queue,
                      int &next_seq_number, const uint64 width,
                      const uint64 height) {
    fprintf(stderr, "[INFO] resolution=%ldx%ld\n", width, height);

    rdt::SEGMENT tmp_seg;
    assert(next_seq_number == 1);
    rdt::initSegment(&tmp_seg, next_seq_number);
    pack_uint64(width, &tmp_seg);
    tmp_seg.header.length = 8;
    rdt::setChecksum(&tmp_seg);
    segment_queue.push_back(tmp_seg);
    next_seq_number++;

    // rdt::SEGMENT height_seg;
    assert(next_seq_number == 2);
    rdt::initSegment(&tmp_seg, next_seq_number);
    pack_uint64(height, &tmp_seg);
    tmp_seg.header.length = 8;
    rdt::setChecksum(&tmp_seg);
    segment_queue.push_back(tmp_seg);
    next_seq_number++;
}

// Fill queue to >= Default_Window_Size
void fill_queue(std::deque<rdt::SEGMENT> &segment_queue,
                const int current_window, int &next_seq_number,
                cv::VideoCapture &cap, cv::Mat &frame_container) {
    while (segment_queue.size() < current_window) {
        // Already finished
        if (!segment_queue.empty() && segment_queue.back().header.fin) {
            return;
        }

        enqueue_frame(segment_queue, next_seq_number, cap, frame_container);
        // // Testing with  string transmission
        // rdt::SEGMENT tmp_seg;
        // rdt::initSegment(&tmp_seg, next_seq_number);
        // printf("Enter message > ");
        // scanf("%s", tmp_seg.data);
        // printf("\n");
        // tmp_seg.header.length = strlen(tmp_seg.data);
        // rdt::setChecksum(&tmp_seg);
        // if (strcmp(tmp_seg.data, "exit") == 0) {
        //     tmp_seg.header.fin = 1;
        // }
        // next_seq_number++;
    }
}

// Transmit a window of segments
void transmit_segments(const std::deque<rdt::SEGMENT> &segment_queue,
                       const int current_window, const int sender_socket,
                       const struct sockaddr_in &agent_addr,
                       socklen_t agent_addr_size) {
    for (int i = 0; i < current_window; ++i) {
        // rdt::printHeader(&segment_queue[i]);
        int sent_bytes =
            sendto(sender_socket, &segment_queue[i], sizeof(segment_queue[i]),
                   0, (struct sockaddr *)&agent_addr, agent_addr_size);
        if (sent_bytes < 0) {
            perror("sendto() error...");
            exit(1);
        } else {
            // fprintf(stderr, "[INFO] sendto() sent %d bytes\n", sent_bytes);
        }
        if (!segment_queue[i].header.fin) {
            printf("send\tdata\t#%d,\twinSize = %d\n",
                   segment_queue[i].header.seqNumber, current_window);
        } else {
            printf("send\tfin\n");
            return;
        }
    }
}

void receive_acks(std::deque<rdt::SEGMENT> &segment_queue, int &SSthreshold,
                  int &current_window, bool &streaming, const int sender_socket,
                  const char agent_ip[], const int agent_port) {
    int ack_count = 0;
    rdt::SEGMENT received_segment;
    struct sockaddr_in source_addr;
    socklen_t source_addr_size = sizeof(source_addr);
    char source_ip[1000];
    int source_port;

    while (ack_count < current_window) {
        memset(&received_segment, 0, sizeof(rdt::SEGMENT));
        int recv_ret =
            recvfrom(sender_socket, &received_segment, sizeof(received_segment),
                     0, (struct sockaddr *)&source_addr, &source_addr_size);

        if (recv_ret < 0) {  // Error, could be timeout
            if (errno != EAGAIN || errno != EWOULDBLOCK) {
                perror("recvfrom() error...");
                exit(1);
            }
            // fprintf(stderr, "[INFO] timeout\n");
            printf("time\tout\n");
            ack_count = 0;
            SSthreshold = (current_window / 2 > 1) ? current_window / 2 : 1;
            current_window = 1;
            return;
        }

        // Throw away packets not from agent
        inet_ntop(AF_INET, &source_addr.sin_addr.s_addr, source_ip,
                  sizeof(source_ip));
        source_port = ntohs(source_addr.sin_port);
        // fprintf(stderr, "[INFO] source ip: %s, source port: %d,
        // addr_size: %d\n", source_ip, source_port, source_addr_size);
        if (strcmp(source_ip, agent_ip) != 0 || source_port != agent_port) {
            fprintf(stderr, "[INFO] received packets not from agent\n");
            continue;
        }

        // Should be either finack or ack
        if (!received_segment.header.ack) {
            fprintf(stderr, "Received non-ACK, exiting...\n");
            exit(1);
        }

        // ACKs don't contain data, so no need for checksum validation

        if (received_segment.header.fin) {  // Finack
            streaming = false;
            printf("recv\tfinack\n");
            return;
        }

        // Normal ack
        int ack_num = received_segment.header.ackNumber;
        printf("recv\tack\t#%d\n", ack_num);
        while (!segment_queue.empty() &&
               segment_queue[0].header.seqNumber <= ack_num) {
            segment_queue.pop_front();
            ack_count++;
        }
    }

    // Congestion control
    bool congestion_avoidance = current_window >= SSthreshold;
    if (congestion_avoidance) {
        current_window++;
    } else {
        current_window *= 2;
    }
}

int main(int argc, char *argv[]) {
    int sender_port, agent_port;
    char sender_ip[32], agent_ip[32], filename[PATH_MAX + 16];
    int sender_socket;
    struct sockaddr_in sender_addr, agent_addr;
    socklen_t sender_addr_size, agent_addr_size;

    // Parse args
    if (argc != 4) {
        fprintf(
            stderr,
            "Usage: %s <sender port> <agent IP>:<agent port> <.mpg filename>\n",
            argv[0]);
        exit(1);
    }
    sscanf(argv[1], "%d", &sender_port);
    snprintf(sender_ip, 32, "127.0.0.1");
    sscanf(argv[2], "%[^:]:%d", agent_ip, &agent_port);
    strncpy(filename, argv[3], PATH_MAX);

    fprintf(stderr, "sender port = %d\n", sender_port);
    fprintf(stderr, "agent ip = %s, agent port = %d\n", agent_ip, agent_port);
    fprintf(stderr, "video to steam = %s\n", filename);

    // Prepare sockets
    if ((sender_socket = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket error...");
        exit(1);
    }
    // unblock_socket(sender_socket);
    rdt::setup_socket_timeout(sender_socket);
    rdt::setup_sockaddr(sender_addr, sender_addr_size, sender_ip, sender_port);
    if (bind(sender_socket, (struct sockaddr *)&sender_addr,
             sizeof(sender_addr)) < 0) {
        perror("bind error...");
        exit(1);
    }

    rdt::setup_sockaddr(agent_addr, agent_addr_size, agent_ip, agent_port);

    // Prepare streaming
    std::deque<rdt::SEGMENT> segment_queue;
    int current_window = rdt::Default_Window_Size;
    int SSthreshold = rdt::Default_SSthreshold;
    int next_seq_number = 1;

    cv::VideoCapture cap(filename);
    cv::Mat frame_container;
    uint64 width = (uint64)cap.get(cv::CAP_PROP_FRAME_WIDTH);
    uint64 height = (uint64)cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    frame_container = cv::Mat::zeros(height, width, CV_8UC3);
    if (!frame_container.isContinuous()) {
        frame_container = frame_container.clone();
    }

    enqueue_metadata(segment_queue, next_seq_number, width, height);

    bool streaming = true;
    // Main transmit loop
    while (streaming) {
        fill_queue(segment_queue, current_window, next_seq_number, cap,
                   frame_container);

        fprintf(stderr,
                "[INFO] Threshold = %d, Current Windows = %d, Congestion "
                "Avoidance = %d\n",
                SSthreshold, current_window, current_window >= SSthreshold);

        transmit_segments(segment_queue, current_window, sender_socket,
                          agent_addr, agent_addr_size);

        // Receive ACKs
        receive_acks(segment_queue, SSthreshold, current_window, streaming,
                     sender_socket, agent_ip, agent_port);
    }

    close(sender_socket);

    return 0;
}