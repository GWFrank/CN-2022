#include "rdt.hpp"

#include <sys/socket.h>

namespace rdt {
    int rdt_sendto(
        int sock_fd,
        const void* msg,
        size_t len,
        const struct sockaddr* dst_addr,
        socklen_t dst_len
    ) {
        return 1;
    }

    int rdt_recvfrom(
        int sock_fd,
        void* buf,
        size_t len,
        const struct sockaddr* src_addr,
        socklen_t src_len
    ) {
        return 1;
    }
}


