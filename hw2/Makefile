# CC = gcc -Wall -O3
CXX = g++ -Wall -O3 -std=c++17
INCLUDE_OPENCV = `pkg-config --cflags --libs opencv4`
LINK_PTHREAD = -lpthread

CLIENT = client.cpp
SERVER = server.cpp
PKT_UTIL = pkt_util.cpp
CMD_UTIL = cmd_util.cpp

CLI = client
SER = server
PKU = pkt_util.o
CMU = cmd_util.o

all: server client
$(SER): $(SERVER) $(PKU) $(CMU)
	$(CXX) $(SERVER) $(PKU) $(CMU) -o $(SER) $(INCLUDE_OPENCV) $(LINK_PTHREAD)
$(CLI): $(CLIENT) $(PKU) $(CMU)
	$(CXX) $(CLIENT) $(PKU) $(CMU) -o $(CLI) $(INCLUDE_OPENCV) $(LINK_PTHREAD)
$(PKU): $(PKT_UTIL) pkt_util.hpp
	$(CXX) -c $(PKT_UTIL) $(INCLUDE_OPENCV)
$(CMU): $(CMD_UTIL) cmd_util.hpp
	$(CXX) -c $(CMD_UTIL) $(INCLUDE_OPENCV)

.PHONY: clean

clean:
	rm $(CLI) $(SER) $(PKU) $(CMU)
