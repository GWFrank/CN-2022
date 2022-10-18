#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <cstring>

#define MAX_BUF_SIZE 2048
#define ERR_EXIT(a){ perror(a); exit(1); }


typedef struct _Packet {
    size_t data_size;
    char* data_p;
} Packet;

int send_packet(int sock_fd, Packet* pkt_p) {
    int write_byte;
    if((write_byte = send(sock_fd, pkt_p->data_p, pkt_p->data_size, MSG_NOSIGNAL)) < 0) {
        // ERR_EXIT("write failed\n");
        perror("write failed\n");
        return 1;
    }
    fprintf(stderr, "[info] Sent %d bytes to fd %d\n", write_byte, sock_fd);
    return 0;
}

int recv_packet(int sock_fd, Packet* pkt_p) {
    int read_byte;
    if((read_byte = read(sock_fd, pkt_p->data_p, pkt_p->data_size)) < 0){
        // ERR_EXIT("read failed\n");
        perror("read failed\n");
        return 1;
    }
    fprintf(stderr, "[info] Received %d bytes from fd %d\n", read_byte, sock_fd);
    return 0;
}

void alloc_packet_size(Packet* pkt_p, size_t sz) {
    if (pkt_p->data_size < sz) {
        if (pkt_p->data_p != NULL)
            free(pkt_p->data_p);
        pkt_p->data_p = (char*)malloc(sz);
        pkt_p->data_size = sz;
    } else {
        pkt_p->data_size = sz;
    }
}

void pack_uint64(uint64_t* src, Packet* pkt_p) {
    alloc_packet_size(pkt_p, sizeof(uint64_t));
    for (int i=0; i<64/8; ++i) {
        pkt_p->data_p[i] = *src >> 8*i;
    }
};

void unpack_uint64(uint64_t* dst, Packet* pkt_p) {
    *dst = 0;
    for (int i=0; i<64/8; ++i) {
        *dst |= (uint64_t)(pkt_p->data_p[i]) << 8*i;
    }
};

void pack_str(char src[], Packet* pkt_p) {
    alloc_packet_size(pkt_p, strlen(src)*sizeof(char));
    memcpy(pkt_p->data_p, src, pkt_p->data_size);
}

void unpack_str(char dst[], Packet* pkt_p) {
    memcpy(dst, pkt_p->data_p, pkt_p->data_size);
    dst[pkt_p->data_size] = 0;
}


int send_str(int sock_fd, char msg[]) {
    Packet pkt = {0, NULL};
    uint64_t msg_len = (uint64_t) strlen(msg);
    // Send message size
    pack_uint64(&msg_len, &pkt);
    if (send_packet(sock_fd, &pkt) == 1) {
        return 1;
    }
    // Send message
    pack_str(msg, &pkt);
    if (send_packet(sock_fd, &pkt) == 1) {
        return 1;
    }
    return 0;
}

int recv_str(int sock_fd, char msg[]) {
    Packet pkt = {0, NULL};
    uint64_t msg_len;
    // Receive message size
    alloc_packet_size(&pkt, sizeof(uint64_t));
    if (recv_packet(sock_fd, &pkt) == 1) {
        return 1;
    }
    unpack_uint64(&msg_len, &pkt);
    // Send message
    alloc_packet_size(&pkt, msg_len*sizeof(char));
    if (recv_packet(sock_fd, &pkt) == 1) {
        return 1;
    }
    unpack_str(msg, &pkt);
    fprintf(stderr, "[info] Received msg: '%s'\n", msg);
    return 0;
}
