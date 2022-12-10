#ifndef RDT_HPP
#define RDT_HPP

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define SEG_DATA_LEN 1000

namespace rdt {
const int Default_SSthreshold = 16;
const int Default_Window_Size = 1;
const int Default_Buffer_Size = 256;
const int Default_Timeout_Seconds = 1;

typedef struct {
    int length;
    int seqNumber;
    int ackNumber;
    int fin;
    int syn;
    int ack;
    unsigned long checksum;
} HEADER;

typedef struct {
    HEADER header;
    char data[SEG_DATA_LEN];
} SEGMENT;

// Initialize all members to 0
void initSegment(SEGMENT* segment, int seq_number);
// Construct an ACK segment
void setACK(SEGMENT* segment, int ack_number, bool ack_fin);
// Calculate and set checksum of data
void setChecksum(SEGMENT* segment);
// Return validity of segment
int validChecksum(const SEGMENT* segment);
// Print segment header
void printHeader(const SEGMENT* segment);

void setup_socket_timeout(int socket_fd);
void unblock_socket(int socket_fd);
void setup_sockaddr(struct sockaddr_in& new_sockaddr, socklen_t& addr_len,
                    char* ip, int port);
}  // namespace rdt

#endif