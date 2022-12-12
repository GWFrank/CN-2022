CXX=g++
CXXFLAGS=-Wall -std=c++17
CXXFLAGS_DEBUG=-Wall -Og -std=c++17
LINK_ZLIB=-lz
INCLUDE_OPENCV=`pkg-config --cflags --libs opencv4`


AGENT_SRC=agent.cpp
SEND_SRC=sender.cpp
RECV_SRC=receiver.cpp
RDT_SRC=rdt.cpp

AGENT_EXEC=agent
SEND_EXEC=sender
RECV_EXEC=receiver
RDT_OBJ=rdt.o

all: agent sender receiver

$(AGENT_EXEC): $(AGENT_SRC) $(RDT_OBJ)
	$(CXX) $(CXXFLAGS) $(AGENT_SRC) $(RDT_OBJ) -o $(AGENT_EXEC) $(LINK_ZLIB)
$(SEND_EXEC): $(SEND_SRC) $(RDT_OBJ)
	$(CXX) $(CXXFLAGS) $(SEND_SRC) $(RDT_OBJ) -o $(SEND_EXEC) $(INCLUDE_OPENCV) $(LINK_ZLIB)
$(RECV_EXEC): $(RECV_SRC) $(RDT_OBJ)
	$(CXX) $(CXXFLAGS) $(RECV_SRC) $(RDT_OBJ) -o $(RECV_EXEC) $(INCLUDE_OPENCV) $(LINK_ZLIB)
$(RDT_OBJ): $(RDT_SRC) rdt.hpp
	$(CXX) $(CXXFLAGS) -c $(RDT_SRC) -o $(RDT_OBJ) $(LINK_ZLIB)

.PHONY: clean

clean:
	rm $(AGENT_EXEC) $(SEND_EXEC) $(RECV_EXEC) $(RDT_OBJ)