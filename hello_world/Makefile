#===============================
# AUTHOR     : Naif Tarafdar
# CREATE DATE     : April 20, 2019
#===============================

XILINX_INCLUDE=../xil_include

DEBUG_FLAGS = -g -DLOG_LEVEL=2
PRODUCTION_FLAGS = -O3 -DLOG_LEVEL=0
BOOST_LDFLAGS=-lboost_thread -lboost_system -lpthread -lboost_log -lboost_log_setup -lboost_program_options
FLAGS=-DCPU  -std=c++17 -isystem $(XILINX_INCLUDE) -I../util/spdlog/include 

release: CXXFLAGS = $(FLAGS)  ${PRODUCTION_FLAGS}
release: hello_world.exe
debug: CXXFLAGS = $(FLAGS) ${DEBUG_FLAGS}
debug: hello_world.exe

all: debug 

hello_world.exe: cpu_node.cpp kern.cpp ../common.cpp
	g++ $(CXXFLAGS) $^ -o $@ $(BOOST_LDFLAGS)	

clean:
	rm -rf *.exe
