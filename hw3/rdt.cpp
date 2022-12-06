#include "rdt.hpp"

#include <zlib.h>

namespace rdt {
    void initHeader(HEADER* header, int seq_number, int ack_number) {
        header->length = 0;
        header->seqNumber = seq_number;
        header->ackNumber = ack_number;
        header->fin = 0;
        header->syn = 0;
        header->ack = 0;
        header->checksum = 0;
    }

    void setChecksum(SEGMENT* seg) {
        seg->header.checksum = crc32(0L, (const Bytef *)seg->data, seg->header.length);
    }

    int validChecksum(const SEGMENT* seg) {
        unsigned long calc_checksum;
        calc_checksum = crc32(0L, (const Bytef *)seg->data, seg->header.length);
        return calc_checksum == seg->header.checksum;
    }
}
