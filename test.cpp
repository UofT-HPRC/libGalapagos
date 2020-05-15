#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include <string>
#include <math.h>
#include <thread>
#include <chrono>

#include "galapagos_interface.hpp"
#include "galapagos_kernel.hpp"
#include "galapagos_local_router.hpp"
#include "galapagos_external_driver.hpp"
#include "galapagos_net_tcp.hpp"
#include "galapagos_node.hpp"

#if LOG_LEVEL==2
#define NUM_ITERATIONS 3 
#else
#define NUM_ITERATIONS 100000
#endif

#if LOG_LEVEL > 0
std::shared_ptr<spdlog::logger> my_logger;
#endif
std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
std::string my_address("10.0.0.1");
std::string remote_address("10.0.0.2");
typedef ap_uint<64> T;
my_barrier barrier(2);

#include "unit_tests/kernel.h"
#include "unit_tests/interface_func.h"
#include "unit_tests/interface_perf.h"
#include "unit_tests/interface_splice_func.h"
#include "unit_tests/interface_splice_perf.h"
#include "unit_tests/interface_trace.h"
#include "unit_tests/kernel_func.h"
#include "unit_tests/kernel_perf.h"
#include "unit_tests/router_kernel_func.h"
#include "unit_tests/router_kernel_perf.h"
#include "unit_tests/node_func.h"
#include "unit_tests/node_perf.h"
// #include "unit_tests/system_func.h"
// #include "unit_tests/system_perf.h"
#include "unit_tests/memory_perf.h"

//************** INPUT GENERATION FUNCTIONS *************************//

 void generate_flit(int size, int id, int dest, galapagos_interface * out){
    galapagos_packet gp;
    gp.id = id;
    gp.dest = dest;
    gp.last = 0;
    start = std::chrono::high_resolution_clock::now();
    for(int j=0; j<NUM_ITERATIONS; j++){
        for(int i=0; i<size; i++){
            ap_uint <32> lower = i;
            ap_uint <32> higher = 0xdeadbeef;
            gp.data = (higher(31, 0), lower(31,0));
            gp.last = (i==(size - 1));
	        out->write(gp);
        }
    }
}

void generate_packet(int size, int id, int dest, galapagos_interface * out){
    galapagos_packet gp;
    gp.id = id;
    gp.dest = dest;
    gp.last = 0;
   
    std::vector<ap_uint<64> > vec(size);
    start = std::chrono::high_resolution_clock::now();
    for(int j=0; j<NUM_ITERATIONS; j++){
        for(int i=0; i<size; i++){
            ap_uint <32> lower = i;
            ap_uint <32> higher = 0xdeadbeef;
            gp.data = (higher(31, 0), lower(31,0));
            gp.last = (i==(size - 1));
            vec[i] = gp.data;
	    }
	    out->packet_write((char *)vec.data(), size, gp.dest, id);
    }
}

//************** OUTPUT PERFORMANCE FUNCTIONS *************************//

void receive_flit_perf(int size, galapagos_interface * in){
    galapagos_packet gp;
    for(int j=0; j<NUM_ITERATIONS; j++){
        for(int i=0; i<size; i++){
            gp = in->read();
        }
    }
    end = std::chrono::high_resolution_clock::now();
}

void receive_packet_perf(int size, galapagos_interface * in){
    galapagos_packet gp;
    size_t packet_size = size;
	short dest;
	short id;
   
    for(int j=0; j<NUM_ITERATIONS; j++){
        ap_uint<64> * ptr = (ap_uint<64> *)in->packet_read(&packet_size, &dest, &id);
    }
    end = std::chrono::high_resolution_clock::now();
}

void receive_packet_mem_perf(int size, galapagos_interface * in){
    galapagos_packet gp;
    size_t packet_size = size;
	short dest;
	short id;

    char* mem = (char *)malloc(MAX_BUFFER*sizeof(gp.data));
   
    for(int j=0; j<NUM_ITERATIONS; j++){
        gp = in->read();
        gp = in->read();
        in->packet_read(mem, &packet_size, &dest, &id);
    }
    end = std::chrono::high_resolution_clock::now();
}

void print_performance(std::string test_name, std::string test_type, int size){
    std::chrono::duration<double> diff = end-start;
    double throughput = ((size*NUM_ITERATIONS*sizeof(ap_uint<64>))/diff.count()/(1000*1000/8)); // Mb/s

    std::cout << test_name << "," << test_type << "," << size << "," << throughput << std::endl;
}


//************** INPUT GENERATION KERNELS *************************//

void kern_generate_flit(short id, galapagos_interface * in, galapagos_interface * out){
    generate_flit(MAX_BUFFER, id, id+1, out);
}

void kern_generate_packet(short id, galapagos_interface * in, galapagos_interface * out){
    generate_packet(MAX_BUFFER, id, id+1, out);
}


//************** OUTPUT VERIFICATION KERNELS *************************//

void kern_output_flit_verify(short id, galapagos_interface * in, galapagos_interface *out){
    
    galapagos_packet gp;
    
    for(int j=0; j<NUM_ITERATIONS; j++){
        for(int i=0; i<MAX_BUFFER; i++){
            gp = in->read();
	    REQUIRE(gp.data.range(63,32) == 0xdeadbeef);
	    REQUIRE(gp.data.range(31,0) == i);
	    REQUIRE(gp.last == (i==(MAX_BUFFER - 1)));	
        }
    }
    end = std::chrono::high_resolution_clock::now();
}

void kern_output_packet_verify(short id, galapagos_interface * in, galapagos_interface *out){
    
    galapagos_packet gp;
    
   
    for(int j=0; j<NUM_ITERATIONS; j++){
	size_t size;
	short dest;
	short id;
        ap_uint<64> * ptr = (ap_uint<64> *)in->packet_read(&size, &dest, &id);
	for(int i=0; i<MAX_BUFFER; i++){
	    REQUIRE(ptr[i].range(63,32) == 0xdeadbeef);
	    REQUIRE(ptr[i].range(31,0) == i);
        }
    }
    end = std::chrono::high_resolution_clock::now();
}

//************** OUTPUT PERFORMANCE KERNELS *************************//

void kern_output_flit_perf(short id, galapagos_interface * in, galapagos_interface *out){
    receive_flit_perf(MAX_BUFFER, in);
}

void kern_output_packet_perf(short id, galapagos_interface * in, galapagos_interface *out){
    receive_packet_perf(MAX_BUFFER, in);
}

//************** BENCHMARK KERNELS *************************//

int msg_size[] = {64, 128, 256, 512, MAX_BUFFER}; // should have 5 elements currently

void kern_benchmark_0(short id, galapagos_interface * in, galapagos_interface * out){

    generate_flit(msg_size[0], id, id+1, out);
    barrier.wait();
    receive_flit_perf(msg_size[1], in);
    print_performance("flit-flit", "perf", msg_size[1]);
    barrier.wait();
    generate_flit(msg_size[2], id, id+1, out);
    barrier.wait();
    receive_flit_perf(msg_size[3], in);
    print_performance("flit-flit", "perf", msg_size[3]);
    barrier.wait();
    generate_flit(msg_size[4], id, id+1, out);
    barrier.wait();

    // generate_packet(msg_size[0], id, id+1, out);
    // barrier.wait();
    // receive_flit_perf(msg_size[1], in);
    // print_performance("packet-flit", "perf", msg_size[1]);
    // barrier.wait();
    // generate_packet(msg_size[2], id, id+1, out);
    // barrier.wait();
    // receive_flit_perf(msg_size[3], in);
    // print_performance("packet-flit", "perf", msg_size[3]);
    // barrier.wait();
    // generate_packet(msg_size[4], id, id+1, out);
    // barrier.wait();

    // generate_flit(msg_size[0], id, id+1, out);
    // barrier.wait();
    // receive_packet_perf(msg_size[1], in);
    // print_performance("flit-packet", "perf", msg_size[1]);
    // barrier.wait();
    // generate_flit(msg_size[2], id, id+1, out);
    // barrier.wait();
    // receive_packet_perf(msg_size[3], in);
    // print_performance("flit-packet", "perf", msg_size[3]);
    // barrier.wait();
    // generate_flit(msg_size[4], id, id+1, out);
    // barrier.wait();

    generate_packet(msg_size[0], id, id+1, out);
    barrier.wait();
    receive_packet_perf(msg_size[1], in);
    print_performance("packet-packet", "perf", msg_size[1]);
    barrier.wait();
    generate_packet(msg_size[2], id, id+1, out);
    barrier.wait();
    receive_packet_perf(msg_size[3], in);
    print_performance("packet-packet", "perf", msg_size[3]);
    barrier.wait();
    generate_packet(msg_size[4], id, id+1, out);
    barrier.wait();

    generate_packet(msg_size[0], id, id+1, out);
    barrier.wait();
    receive_packet_mem_perf(msg_size[1], in);
    print_performance("packet-packet_mem", "perf", msg_size[1]);
    barrier.wait();
    generate_packet(msg_size[2], id, id+1, out);
    barrier.wait();
    receive_packet_mem_perf(msg_size[3], in);
    print_performance("packet-packet_mem", "perf", msg_size[3]);
    barrier.wait();
    generate_packet(msg_size[4], id, id+1, out);
    barrier.wait();
    
}

void kern_benchmark_1(short id, galapagos_interface * in, galapagos_interface *out){
    
    receive_flit_perf(msg_size[0], in);
    print_performance("flit-flit", "perf", msg_size[0]);
    barrier.wait();
    generate_flit(msg_size[1], id, id-1, out);
    barrier.wait();
    receive_flit_perf(msg_size[2], in);
    print_performance("flit-flit", "perf", msg_size[2]);
    barrier.wait();
    generate_flit(msg_size[3], id, id-1, out);
    barrier.wait();
    receive_flit_perf(msg_size[4], in);
    print_performance("flit-flit", "perf", msg_size[4]);
    barrier.wait();

    // receive_flit_perf(msg_size[0], in);
    // print_performance("packet-flit", "perf", msg_size[0]);
    // barrier.wait();
    // generate_packet(msg_size[1], id, id-1, out);
    // barrier.wait();
    // receive_flit_perf(msg_size[2], in);
    // print_performance("packet-flit", "perf", msg_size[2]);
    // barrier.wait();
    // generate_packet(msg_size[3], id, id-1, out);
    // barrier.wait();
    // receive_flit_perf(msg_size[4], in);
    // print_performance("packet-flit", "perf", msg_size[4]);
    // barrier.wait();

    // receive_packet_perf(msg_size[0], in);
    // print_performance("flit-packet", "perf", msg_size[0]);
    // barrier.wait();
    // generate_flit(msg_size[1], id, id-1, out);
    // barrier.wait();
    // receive_packet_perf(msg_size[2], in);
    // print_performance("flit-packet", "perf", msg_size[2]);
    // barrier.wait();
    // generate_flit(msg_size[3], id, id-1, out);
    // barrier.wait();
    // receive_packet_perf(msg_size[4], in);
    // print_performance("flit-packet", "perf", msg_size[4]);
    // barrier.wait();

    receive_packet_perf(msg_size[0], in);
    print_performance("packet-packet", "perf", msg_size[0]);
    barrier.wait();
    generate_packet(msg_size[1], id, id-1, out);
    barrier.wait();
    receive_packet_perf(msg_size[2], in);
    print_performance("packet-packet", "perf", msg_size[2]);
    barrier.wait();
    generate_packet(msg_size[3], id, id-1, out);
    barrier.wait();
    receive_packet_perf(msg_size[4], in);
    print_performance("packet-packet", "perf", msg_size[4]);
    barrier.wait();

    receive_packet_mem_perf(msg_size[0], in);
    print_performance("packet-packet_mem", "perf", msg_size[0]);
    barrier.wait();
    generate_packet(msg_size[1], id, id-1, out);
    barrier.wait();
    receive_packet_mem_perf(msg_size[2], in);
    print_performance("packet-packet_mem", "perf", msg_size[2]);
    barrier.wait();
    generate_packet(msg_size[3], id, id-1, out);
    barrier.wait();
    receive_packet_mem_perf(msg_size[4], in);
    print_performance("packet-packet_mem", "perf", msg_size[4]);
    barrier.wait();
    
}

void axis_fifo(galapagos_interface * s_axis, galapagos_interface * m_axis, int count){

    for(int i=0; i<count; i++){
        m_axis->splice(s_axis);	
    }

}




void send_single_flit(short id, galapagos_interface * in, galapagos_interface *out){
    galapagos_packet gp;
    gp.data = 42;
    gp.id = id;
    gp.dest = id+1;
    gp.last = 1;
    out->write(gp);
}

void send_single_packet(short id, galapagos_interface * in, galapagos_interface *out){
    galapagos_packet gp;
    gp.data = 42;
    gp.id = id;
    gp.dest = id+1;
    gp.last = 1;
    out->packet_write((char *)&gp.data, sizeof(T), gp.dest, gp.id);
}


void recv_single_flit(short id, galapagos_interface * in, galapagos_interface *out){
    galapagos_packet gp = in->read();
	REQUIRE(gp.data == 42);
}    

void recv_single_packet(short id, galapagos_interface * in, galapagos_interface *out){
    size_t size;
    short dest;
    short recv_id;
    ap_uint<64> * ptr = (ap_uint<64> *)in->packet_read(&size, &dest, &recv_id);
    free(ptr);
}    

void kern_flit_loopback_perf(short id, galapagos_interface * in, galapagos_interface *out){
    recv_single_flit(id, in, out);

    galapagos_packet gp;

    start = std::chrono::high_resolution_clock::now();
    for(int j=0; j<NUM_ITERATIONS; j++){
        for(int i=0; i<MAX_BUFFER; i++){
	    gp = in->read();
	    gp.dest = gp.id;
	    gp.id = id;
	    out->write(gp);
	}
    }
    end = std::chrono::high_resolution_clock::now();
}

void kern_flit_loopback_verify(short id, galapagos_interface * in, galapagos_interface *out){
    recv_single_flit(id, in, out);

    galapagos_packet gp;

    start = std::chrono::high_resolution_clock::now();
    for(int j=0; j<NUM_ITERATIONS; j++){
        for(int i=0; i<MAX_BUFFER; i++){
	    gp = in->read();
	    REQUIRE(gp.data.range(63,32) == 0xdeadbeef);
	    REQUIRE(gp.data.range(31,0) == i);
	    REQUIRE(gp.last == (i==(MAX_BUFFER - 1)));	
	    gp.dest = gp.id;
	    gp.id = id;
	    out->write(gp);
	}
    }
    end = std::chrono::high_resolution_clock::now();
}

void kern_packet_loopback_perf(short id, galapagos_interface * in, galapagos_interface *out){
    recv_single_flit(id, in, out);

    galapagos_packet gp;

    start = std::chrono::high_resolution_clock::now();
    for(int j=0; j<NUM_ITERATIONS; j++){
	size_t size;
	short dest;
	short recv_id;
        ap_uint<64> * ptr = (ap_uint<64> *)in->packet_read(&size, &dest, &recv_id);
	out->packet_write((char *)ptr, size, recv_id, id);
    	free(ptr);
    }
    end = std::chrono::high_resolution_clock::now();
}

void kern_packet_loopback_verify(short id, galapagos_interface * in, galapagos_interface *out){
    recv_single_flit(id, in, out);

    galapagos_packet gp;

    start = std::chrono::high_resolution_clock::now();
    for(int j=0; j<NUM_ITERATIONS; j++){
	    size_t size;
	    short dest;
	    short recv_id;
        ap_uint<64> * ptr = (ap_uint<64> *)in->packet_read(&size, &dest, &recv_id);
	    for(int i=0; i<MAX_BUFFER; i++){
	        REQUIRE(ptr[i].range(63,32) == 0xdeadbeef);
	        REQUIRE(ptr[i].range(31,0) == i);
        }
	    out->packet_write((char *)ptr, size, recv_id, id);
    	free(ptr);
    }
    end = std::chrono::high_resolution_clock::now();
}

void kern_generate_output_flit_verify(short id, galapagos_interface * in, galapagos_interface *out){

    send_single_flit(id, in, out);
    kern_generate_flit(id, in, out);
    kern_output_flit_verify(id, in, out);
}


void kern_generate_output_packet_verify(short id, galapagos_interface * in, galapagos_interface *out){


    send_single_flit(id, in, out);
    kern_generate_packet(id, in, out);
    kern_output_packet_verify(id, in, out);

}

void kern_generate_output_flit_perf(short id, galapagos_interface * in, galapagos_interface *out){

    send_single_flit(id, in, out);
    kern_generate_flit(id, in, out);
    kern_output_flit_perf(id, in, out);
}


void kern_generate_output_packet_perf(short id, galapagos_interface * in, galapagos_interface *out){

    send_single_flit(id, in, out);
    kern_generate_packet(id, in, out);
    kern_output_packet_perf(id, in, out);

}


int main(int argc, char * argv[]){
    
    #if LOG_LEVEL > 0
    my_logger = spdlog::basic_logger_mt("basic_logger", "test.log");
    #endif 
// #if LOG_LEVEL==0
//     spdlog::set_level(spdlog::level::off); // Set global log level to off
    #if LOG_LEVEL==1
    spdlog::set_level(spdlog::level::info); // Set global log level to info
    #elif LOG_LEVEL==2
    spdlog::set_level(spdlog::level::debug); // Set global log level to debug
    #endif
    #if LOG_LEVEL > 0
    spdlog::flush_every(std::chrono::seconds(2));
    my_logger->info("Starting Unit Tests");
    my_logger->flush();
    #endif
    int result = Catch::Session().run(argc, argv);

}
