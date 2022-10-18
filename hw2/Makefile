CC = gcc -Wall
CXX = g++ -Wall
INCLUDE_OPENCV = `pkg-config --cflags --libs opencv4`
LINK_PTHREAD = -lpthread

CLIENT = client.cpp
SERVER = server.cpp
OPEN_CV = openCV.cpp
PTHREAD = pthread.cpp
CLI = client
SER = server
CV = openCV
PTH = pthread

all: server client opencv pthread
  
server: $(SERVER)
	$(CXX) $(SERVER) -o $(SER) $(INCLUDE_OPENCV) $(LINK_PTHREAD)
client: $(CLIENT)
	$(CXX) $(CLIENT) -o $(CLI)
opencv: $(OPEN_CV)
	$(CXX) $(OPEN_CV) -o $(CV) $(INCLUDE_OPENCV)
pthread: $(PTHREAD)
	$(CXX) $(PTHREAD) -o $(PTH) $(LINK_PTHREAD)

.PHONY: clean

clean:
	rm $(CLI) $(SER) $(CV) $(PTH)
