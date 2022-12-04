#ifndef PKT_UTIL_HPP
#define PKT_UTIL_HPP

#include <opencv2/opencv.hpp>

#include <sys/socket.h>
#include <cstdint>

#define MAX_BUF_SIZE 2048
#define ERR_EXIT(a){ perror(a); exit(1); }

namespace pku {
    struct Packet {
        size_t data_size;
        uint8_t* data_p;
        Packet(): data_size(0), data_p(NULL) {}
    };

    const int FILE_SEG_SIZE = 1<<20;

    // Return 0 when success, non-0 when error
    int send_packet(
        const int sock_fd,
        Packet &pkt,
        const struct sockaddr* dst_addr,
        socklen_t dst_len
    );

    // Return 0 when success, non-0 when error
    int recv_packet(
        const int sock_fd,
        Packet &pkt,
        size_t sz,
        const struct sockaddr* src_addr,
        socklen_t src_len
    );

    void alloc_packet_size(Packet &pkt, size_t sz);

    void pack_uint64(const uint64_t src, Packet &pkt);
    void pack_int64(const int64_t src, Packet &pkt);
    void pack_str(const char src[], Packet &pkt);
    void pack_double(const double &src, Packet &pkt);
    void pack_frame(const cv::Mat &img, Packet &pkt);

    void unpack_uint64(uint64_t &dst, Packet &pkt);
    void unpack_int64(int64_t &dst, Packet &pkt);
    void unpack_str(char dst[], Packet &pkt);
    void unpack_double(double &dst, Packet &pkt);
    void unpack_frame(cv::Mat &img, Packet &pkt);

    // Return 0 when success, non-0 when error
    int send_int64(
        const int sock_fd,
        const int64_t &num,
        const struct sockaddr* dst_addr,
        socklen_t dst_len
    );

    // Return 0 when success, non-0 when error
    int recv_int64(
        const int sock_fd,
        int64_t &num,
        const struct sockaddr* src_addr,
        socklen_t src_len
    );

    // Return 0 when success, non-0 when error
    int send_str(
        const int sock_fd,
        const char msg[],
        const struct sockaddr* dst_addr,
        socklen_t dst_len
    );

    // Return 0 when success, non-0 when error
    int recv_str(
        const int sock_fd,
        char msg[],
        const struct sockaddr* src_addr,
        socklen_t src_len
    );

    // Return received frame size
    int64_t send_frame(
        const int sock_fd,
        cv::Mat &img,
        Packet &pkt,
        const struct sockaddr* dst_addr,
        socklen_t dst_len
    );

    // Return received frame size
    int64_t recv_frame(
        const int sock_fd,
        cv::Mat &img,
        Packet &pkt,
        const struct sockaddr* src_addr,
        socklen_t src_len
    );

    // Return 0 when success, non-0 when error
    int send_video(
        const int sock_fd,
        const char video_path[],
        const struct sockaddr* receiver_addr,
        socklen_t receiver_len
    );

    // Return 0 when success, non-0 when error
    int recv_video(
        const int sock_fd,
        const struct sockaddr* sender_addr,
        socklen_t sender_len
    );
}


#endif // PKT_UTIL_HPP
