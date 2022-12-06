#ifndef RDT_HPP
#define RDT_HPP

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
    void initHeader(HEADER* header, int seq_number, int ack_number);
    // Calculate and set checksum of data
    void setChecksum(SEGMENT* seg);
    // Return validity of segment
    int validChecksum(const SEGMENT* seg);

}

#endif