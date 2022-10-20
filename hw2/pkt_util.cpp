#include "pkt_util.hpp"

#include <opencv2/opencv.hpp>

#include <chrono>

#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>


int send_packet(const int sock_fd, Packet &pkt) {
    int write_byte;
    if((write_byte = send(sock_fd, pkt.data_p, pkt.data_size, MSG_NOSIGNAL)) < 0) {
        // ERR_EXIT("write failed\n");
        perror("write failed\n");
        return 1;
    }
    // fprintf(stderr, "[info] Sent %d bytes to fd %d\n", write_byte, sock_fd);
    return 0;
}
int recv_packet(const int sock_fd, Packet &pkt, size_t sz) {
    alloc_packet_size(pkt, sz);
    int read_byte=0;
    while (sz>0) {
        // fprintf(stderr, "[debug] %ld bytes left to read\n", sz);
        if((read_byte = read(sock_fd, pkt.data_p + (pkt.data_size-sz), sz)) < 0){
            // ERR_EXIT("read failed\n");
            perror("read failed\n");
            return 1;
        } else {
            sz -= read_byte;
        }
    }
    // fprintf(stderr, "[info] Received %d bytes from fd %d\n", sz, sock_fd);
    return 0;
}
void alloc_packet_size(Packet &pkt, size_t sz) {
    if (pkt.data_size < sz) {
        if (pkt.data_p != NULL)
            free(pkt.data_p);
        pkt.data_p = (uint8_t*)malloc(sz);
    }
    pkt.data_size = sz;
}

void pack_uint64(const uint64_t src, Packet &pkt) {
    alloc_packet_size(pkt, sizeof(uint64_t));
    for (int i=0; i<64/8; ++i) {
        pkt.data_p[i] = src >> 8*i;
    }
}
void pack_int64(const int64_t src, Packet &pkt) {
    alloc_packet_size(pkt, sizeof(int64_t));
    for (int i=0; i<64/8; ++i) {
        pkt.data_p[i] = src >> 8*i;
    }
}
void pack_str(const char src[], Packet &pkt) {
    alloc_packet_size(pkt, strlen(src)*sizeof(char));
    memcpy(pkt.data_p, src, pkt.data_size);
}
void pack_double(const double &src, Packet &pkt) {
    alloc_packet_size(pkt, sizeof(double));
    uint64_t src_int = *((uint64_t*)&src);
    for (int i=0; i<64/8; ++i) {
        pkt.data_p[i] = src_int >> 8*i;
    }
}
void pack_frame(const cv::Mat &img, Packet &pkt) {
    alloc_packet_size(pkt, img.total() * img.elemSize());
    memcpy(pkt.data_p, img.data, pkt.data_size);
}

void unpack_uint64(uint64_t &dst, Packet &pkt) {
    dst = 0;
    for (int i=0; i<64/8; ++i) {
        dst |= (uint64_t)(pkt.data_p[i]) << 8*i;
    }
}
void unpack_int64(int64_t &dst, Packet &pkt) {
    dst = 0;
    for (int i=0; i<64/8; ++i) {
        dst |= (int64_t)(pkt.data_p[i]) << 8*i;
    }
}
void unpack_str(char dst[], Packet &pkt) {
    memcpy(dst, pkt.data_p, pkt.data_size);
    dst[pkt.data_size] = 0;
}
void unpack_double(double &dst, Packet &pkt) {
    uint64_t dst_int = 0;
    for (int i=0; i<64/8; ++i) {
        dst_int |= (uint64_t)(pkt.data_p[i]) << 8*i;
    }
    dst = *((double*)&dst_int);
}
void unpack_frame(cv::Mat &img, Packet &pkt) {
    memcpy(img.data, pkt.data_p, pkt.data_size);
}

int send_str(const int sock_fd, char msg[]) {
    Packet pkt;
    uint64_t msg_len = (uint64_t) strlen(msg);
    // Send message size
    pack_uint64(msg_len, pkt);
    if (send_packet(sock_fd, pkt) == 1) {
        goto send_str_err;
    }
    // fprintf(stderr, "[info] Sent payload length = %ld\n", msg_len);
    // Send message
    pack_str(msg, pkt);
    if (send_packet(sock_fd, pkt) == 1) {
        goto send_str_err;
    }
    // fprintf(stderr, "[info] Sent payload\n");

    if (pkt.data_p != NULL)
        free(pkt.data_p);
    return 0;
send_str_err:
    if (pkt.data_p != NULL)
        free(pkt.data_p);
    return 1;
}

int recv_str(const int sock_fd, char msg[]) {
    Packet pkt;
    uint64_t msg_len;
    // Receive message size
    // alloc_packet_size(pkt, sizeof(uint64_t));
    if (recv_packet(sock_fd, pkt, sizeof(uint64_t)) == 1) {
        goto recv_str_err;
    }
    unpack_uint64(msg_len, pkt);
    // fprintf(stderr, "[info] Received payload length = %ld\n", msg_len);
    // Receive message
    // alloc_packet_size(pkt, msg_len*sizeof(char));
    if (recv_packet(sock_fd, pkt, msg_len*sizeof(char)) == 1) {
        goto recv_str_err;
    }
    unpack_str(msg, pkt);
    // fprintf(stderr, "[info] Received payload\n");
    fprintf(stderr, "[info] Received msg: '%s'\n", msg);

    if (pkt.data_p != NULL)
        free(pkt.data_p);
    return 0;
recv_str_err:
    if (pkt.data_p != NULL)
        free(pkt.data_p);
    return 1;
}

int64_t send_frame(const int sock_fd, cv::Mat &img, Packet &pkt) {
    int64_t img_size = img.total() * img.elemSize();
    // Send frame size, this will be 0 at the end of video
    pack_int64(img_size, pkt);
    if (send_packet(sock_fd, pkt) == 1) {
        return -1;
    }
    // Send frame
    pack_frame(img, pkt);
    if (send_packet(sock_fd, pkt) == 1) {
        return -1;
    }
    return img_size;
}

int64_t recv_frame(const int sock_fd, cv::Mat &img, Packet &pkt) {
    int64_t img_size = -1;
    // Receive frame size, this will be 0 at the end of video
    if (recv_packet(sock_fd, pkt, sizeof(uint64_t)) == 1) {
        return -1;
    }
    unpack_int64(img_size, pkt);
    // fprintf(stderr, "[info] Received frame length = %ld\n", img_size);
    // Receive frame
    if (recv_packet(sock_fd, pkt, img_size) == 1) {
        return -1;
    }
    unpack_frame(img, pkt);
    // fprintf(stderr, "[info] Received frame of size %ld bytes\n", img_size);
    return img_size;
}

int send_video(const int sock_fd, const char video_path[]) {
    Packet pkt;
    cv::VideoCapture cap(video_path);
    uint64_t width = (uint64_t)cap.get(cv::CAP_PROP_FRAME_WIDTH);
    uint64_t height = (uint64_t)cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    double frame_time = 1000/cap.get(cv::CAP_PROP_FPS);
    cv::Mat server_img;
    int count;
    // Send resolution and frame time
    pack_uint64(width, pkt);
    if (send_packet(sock_fd, pkt) == 1) {
        goto send_video_err;
    }
    pack_uint64(height, pkt);
    if (send_packet(sock_fd, pkt) == 1) {
        goto send_video_err;
    }
    pack_double(frame_time, pkt);
    if (send_packet(sock_fd, pkt) == 1) {
        goto send_video_err;
    }
    // Prepare container for frames
    server_img = cv::Mat::zeros(height, width, CV_8UC3);
    if (!server_img.isContinuous()) {
        server_img = server_img.clone();
    }

    count = 0;
    while (1) {
        // Send video frame
        cap >> server_img;
        int64_t ret = send_frame(sock_fd, server_img, pkt);
        if (ret < 0) {
            goto send_video_err;
        } else if (ret == 0) { // End of video
            break;
        }
        count++;
        // Receive key pressed
        int64_t key_pressed;
        // alloc_packet_size(pkt, sizeof(int64_t));
        if (recv_packet(sock_fd, pkt, sizeof(int64_t)) == 1) {
            goto send_video_err;
        }
        unpack_int64(key_pressed, pkt);
        if (key_pressed == 27)
            break;
    }
    fprintf(stderr, "[video] Successfully sent %d frames\n", count);
    
    cap.release();
    if (pkt.data_p != NULL)
        free(pkt.data_p);
    return 0;
send_video_err:
    cap.release();
    if (pkt.data_p != NULL)
        free(pkt.data_p);
    return 1;
}

int recv_video(const int sock_fd) {
    Packet pkt;
    uint64_t width;
    uint64_t height;
    double frame_time;
    cv::Mat client_img;
    int frame_count;

    std::chrono::duration<double> video_time;
    auto video_st = std::chrono::system_clock::now();
    auto ts_prev = std::chrono::system_clock::now();
    auto ts_intvl = std::chrono::milliseconds(0);
    int64_t ret, key_pressed;

    // Receive resolution and frame time
    if (recv_packet(sock_fd, pkt, sizeof(uint64_t)) == 1) {
        goto recv_video_err;
    }
    unpack_uint64(width, pkt);
    if (recv_packet(sock_fd, pkt, sizeof(uint64_t)) == 1) {
        goto recv_video_err;
    }
    unpack_uint64(height, pkt);
    if (recv_packet(sock_fd, pkt, sizeof(uint64_t)) == 1) {
        goto recv_video_err;
    }
    unpack_double(frame_time, pkt);
    fprintf(stderr, "[video] Video metadata: %lux%lu, %.2f FPS\n", width, height, 1000/frame_time);
    // Prepare container for frames
    client_img = cv::Mat::zeros(height, width, CV_8UC3);
    if (!client_img.isContinuous()) {
        client_img = client_img.clone();
    }
    // Receive video frame loop
    video_st = std::chrono::system_clock::now();
    ts_prev = std::chrono::system_clock::now();
    ts_intvl = std::chrono::milliseconds((int64_t)frame_time);
    frame_count = 0;
    while (1) {
        ret = recv_frame(sock_fd, client_img, pkt);
        if (ret < 0) {
            goto recv_video_err;
        } else if (ret == 0) {
            break;
        }
        frame_count++;
        imshow("Video", client_img);
        // Send key pressed
        // int64_t key_pressed = (int64_t)cv::waitKey(0); // debug
        key_pressed = -1;
        while (std::chrono::system_clock::now() - ts_prev < ts_intvl
               && (key_pressed = (int64_t)cv::waitKey(1)) != 27) {   
        }
        ts_prev = std::chrono::system_clock::now();
        pack_int64(key_pressed, pkt);
        if (send_packet(sock_fd, pkt) == 1) {
            goto recv_video_err;
        }
        if (key_pressed == 27)
            break;
    }
    video_time = std::chrono::system_clock::now() - video_st;
    fprintf(stderr, "[video] Successfully received %d frames in %.2f s\n", frame_count, video_time.count());
    fprintf(stderr, "[video] Average frame rate: %.2f FPS\n", frame_count/video_time.count());
    
    cv::destroyAllWindows();
    if (pkt.data_p != NULL)
        free(pkt.data_p);
    return 0;
recv_video_err:
    cv::destroyAllWindows();
    if (pkt.data_p != NULL)
        free(pkt.data_p);
    return 1;
}
