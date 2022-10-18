CC = gcc -Wall
CXX = g++ -Wall
INCLUDE_OPENCV = `pkg-config --cflags --libs opencv4`
LINK_PTHREAD = -lpthread

CLIENT = client.cpp
SERVER = server.cpp server.h
OPEN_CV = openCV.cpp
PTHREAD = pthread.cpp
PKT_UTIL = pkt_util.h
CLI = client
SER = server
CV = openCV
PTH = pthread

all: server client
  
server: $(SERVER) $(PKT_UTIL)
	$(CXX) $(SERVER) -o $(SER) $(INCLUDE_OPENCV) $(LINK_PTHREAD)
client: $(CLIENT) $(PKT_UTIL)
	$(CXX) $(CLIENT) -o $(CLI) $(INCLUDE_OPENCV) $(LINK_PTHREAD)
opencv: $(OPEN_CV)
	$(CXX) $(OPEN_CV) -o $(CV) $(INCLUDE_OPENCV)
pthread: $(PTHREAD)
	$(CXX) $(PTHREAD) -o $(PTH) $(LINK_PTHREAD)

.PHONY: clean

clean:
	rm $(CLI) $(SER) $(CV) $(PTH)
