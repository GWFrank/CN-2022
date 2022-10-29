# CC = gcc -Wall -O3
CXX = g++ -Wall -O3 -std=c++17
INCLUDE_OPENCV = `pkg-config --cflags --libs opencv4`
LINK_PTHREAD = -lpthread

CLIENT = client.cpp
SERVER = server.cpp
VIDEO_TEST = video-test.cpp
PTHREAD = pthread.cpp
PKT_UTIL = pkt_util.cpp
CMD_UTIL = cmd_util.cpp

CLI = client
SER = server
VT = video-test
PTH = pthread
PKU = pkt_util.o
CMU = cmd_util.o

all: server client
$(SER): $(SERVER) $(PKU) $(CMU)
	$(CXX) $(SERVER) $(PKU) $(CMU) -o $(SER) $(INCLUDE_OPENCV) $(LINK_PTHREAD)
$(CLI): $(CLIENT) $(PKU) $(CMU)
	$(CXX) $(CLIENT) $(PKU) $(CMU) -o $(CLI) $(INCLUDE_OPENCV) $(LINK_PTHREAD)
$(PKU): $(PKT_UTIL)
	$(CXX) -c $(PKT_UTIL) $(INCLUDE_OPENCV)
$(CMU): $(CMD_UTIL)
	$(CXX) -c $(CMD_UTIL) $(INCLUDE_OPENCV)

$(VT): $(VIDEO_TEST)
	$(CXX) $(VIDEO_TEST) -o $(VT) $(INCLUDE_OPENCV) $(LINK_PTHREAD)
$(PTH): $(PTHREAD)
	$(CXX) $(PTHREAD) -o $(PTH) $(LINK_PTHREAD)

.PHONY: clean

clean:
	rm $(CLI) $(SER) $(PKU) $(CMU) $(VT) $(PTH)
