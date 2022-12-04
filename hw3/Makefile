CXX = g++
CXXFLAGS = -Wall -O3 -std=c++17
CXXFLAGS_DEBUG = -Wall -Og -std=c++17
LINK_ZLIB = -lz
INCLUDE_OPENCV = `pkg-config --cflags --libs opencv4`

COMMON_SRC = segment.h
AGENT_SRC = agent.cpp
SEND_SRC = sender.cpp
RECV_SRC = receiver.cpp
PKU_SRC = pkt_util.cpp pkt_util.hpp
RDT_SRC = rdt.cpp rdt.hpp

AGENT_EXEC = agent
SEND_EXEC = sender
RECV_EXEC = receiver
PKU_OBJ = pkt_util.o
RDT_OBJ = rdt.o

CRC32_SRC = crc32.cpp
CRC32_EXEC = crc32

all: agent send recv

agent: $(AGENT_SRC) $(COMMON_SRC)
	$(CXX) $(CXXFLAGS) $(AGENT_SRC) -o $(AGENT_EXEC)
send: $(SEND_SRC) $(COMMON_SRC) pku rdt
	$(CXX) $(CXXFLAGS) $(SEND_SRC) $(PKU_OBJ) $(RDT_OBJ) -o $(SEND_EXEC) $(INCLUDE_OPENCV) $(LINK_ZLIB)
recv: $(RECV_SRC) $(COMMON_SRC) pku rdt
	$(CXX) $(CXXFLAGS) $(RECV_SRC) $(PKU_OBJ) $(RDT_OBJ) -o $(RECV_EXEC) $(INCLUDE_OPENCV) $(LINK_ZLIB)
pku: $(PKU_SRC)
	$(CXX) $(CXXFLAGS) -c $(PKU_OBJ) $(INCLUDE_OPENCV) $(LINK_ZLIB)
rdt: $(RDT_SRC) $(COMMON_SRC)
	$(CXX) $(CXXFLAGS) -c $(RDT_OBJ) $(INCLUDE_OPENCV) $(LINK_ZLIB)
crc32:
	$(CXX) $(CRC32_SRC) -o $(CRC32_EXEC) $(LINK_ZLIB)

.PHONY: clean

clean:
	rm $(AGENT_EXEC) $(CRC32_EXEC)