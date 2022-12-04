#ifndef RDT_HPP
#define RDT_HPP

#include "segment.h"

#include <sys/socket.h>

namespace rdt {
    // Return 0 when success, non-0 when error
    int rdt_sendto(
        int sock_fd,
        const void* msg,
        size_t len,
        const struct sockaddr* dst_addr,
        socklen_t dst_len
    );

    // Return 0 when success, non-0 when error
    int rdt_recvfrom(
        int sock_fd,
        void* buf,
        size_t len,
        const struct sockaddr* src_addr,
        socklen_t src_len
    );
}

#endif