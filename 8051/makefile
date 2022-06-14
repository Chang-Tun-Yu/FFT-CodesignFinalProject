#Makefile
LIB_DIR=-L/usr/local/systemc-2.3.3/
INC_DIR=-I/usr/local/systemc-2.3.3/include
LIB=-lsystemc -lncurses
SOURCES = $(wildcard *.cpp)

export SYSTEMC_HOME=/usr/local/systemc-2.3.3/
export LD_LIBRARY_PATH=$(SYSTEMC_HOME)/lib-linux64

all:8051_iss.cpp
	g++ -o 8051_iss $(SOURCES) $(LIB_DIR) $(INC_DIR) $(LIB) -std=c++14
clean:
	rm -f 8051_iss