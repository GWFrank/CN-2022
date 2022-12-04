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
    char data[1000];
} SEGMENT;
