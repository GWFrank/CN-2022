#include "rdt.hpp"
#include "segment.h"

#include <sys/socket.h>

namespace rdt {
    int SS_THRESH = 16;
    int WDW_SZ = 1;
    int SEG_BUF_SZ = 256;
    int TIMEOUT = 1;

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
        struct sockaddr* src_addr,
        socklen_t* src_len
    ) {
        return 1;
    }
}


