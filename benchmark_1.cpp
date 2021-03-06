/*

This set of benchmarks are used to establish the baseline communication performance
of libGalapagos. There are two kinds: the regular set and the ones with _reply.
Choose by passing the appropriate function pointers to the kernel.

This file represents the kernel 1 and benchmark_0.cpp represents kernel 0

The results of this experiment can be seen in Varun Sharma's master's thesis:
PGAS Communication for Heterogeneous Clusters with FPGAs, section 5.3.1

*/

#include <string>
#include <vector>

#include "galapagos_interface.hpp"
#include "galapagos_node.hpp"
#include "galapagos_net_tcp.hpp"

// #include "test.cpp"

#if LOG_LEVEL==2
#define NUM_ITERATIONS 3 
#else
#define NUM_ITERATIONS 100000
#endif

#if LOG_LEVEL > 0
std::shared_ptr<spdlog::logger> my_logger;
#endif
std::chrono::time_point<std::chrono::high_resolution_clock> start, end;

//************** INPUT GENERATION FUNCTIONS *************************//

void generate_flit(int iterations, int size, int id, int dest, galapagos_interface * out){
    galapagos_packet gp;
    gp.id = id;
    gp.dest = dest;
    gp.last = 0;
    for(int j=0; j<iterations; j++){
        for(int i=0; i<size; i++){
            ap_uint <32> lower = i;
            ap_uint <32> higher = 0xdeadbeef;
            gp.data = (higher(31, 0), lower(31,0));
            gp.last = (i==(size - 1));
	        out->write(gp);
        }
    }
}

auto start_timer(){
    return std::chrono::high_resolution_clock::now();
}

void print_time(std::chrono::high_resolution_clock::time_point timer, std::string label){
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = now - timer;
    auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();
    std::cout << label << ":" << elapsed_ns << std::endl;
}

void generate_packet(int iterations, int size, int id, int dest, galapagos_interface * out){
    galapagos_packet gp;
    gp.id = id;
    gp.dest = dest;
    gp.last = 0;
   
    std::vector<ap_uint<64> > vec(size);
    for(int j=0; j<iterations; j++){
        // auto timer = start_timer();
        for(int i=0; i<size; i++){
            ap_uint <32> lower = i;
            ap_uint <32> higher = 0xdeadbeef;
            gp.data = (higher(31, 0), lower(31,0));
            gp.last = (i==(size - 1));
            vec[i] = gp.data;
	    }
        // print_time(timer, "kernel_write_0");
        // timer = start_timer();
	    out->packet_write((char *)vec.data(), size, gp.dest, id);
        // print_time(timer, "kernel_write_1");
        // for(int z = 0; z < iterations/10; z++){
        //     __asm__ __volatile__ ("" : "+g" (z) : : );
        // }
    }
}

void generate_packet(char* mem, int iterations, int size, int id, int dest, galapagos_interface * out){
    for(int j=0; j<iterations; j++){
	    out->packet_write(mem, size, dest, id);
        // for(int z = 0; z < iterations/10; z++){
        //     __asm__ __volatile__ ("" : "+g" (z) : : );
        // }
    }
}

void generate_packet(std::vector<ap_uint<64> >* vec, int iterations, int size, int id, int dest, galapagos_interface * out){
    for(int j=0; j<iterations; j++){
	    out->packet_write((char *)vec->data(), size, dest, id);
        // for(int z = 0; z < iterations/10; z++){
        //     __asm__ __volatile__ ("" : "+g" (z) : : );
        // }
    }
}

//************** OUTPUT PERFORMANCE FUNCTIONS *************************//

void receive_flit_perf(int iterations, int size, galapagos_interface * in){
    galapagos_packet gp;
    for(int j=0; j<iterations; j++){
        for(int i=0; i<size; i++){
            gp = in->read();
        }
    }
}

void receive_packet_perf(int iterations, int size, galapagos_interface * in){
    galapagos_packet gp;
    size_t packet_size;
	short dest;
	short id;
   
    for(int j=0; j<iterations; j++){
        ap_uint<64> * ptr = (ap_uint<64> *)in->packet_read(&packet_size, &dest, &id);
        free(ptr);
    }
}

void receive_packet_mem_perf(char* mem, int iterations, int size, galapagos_interface * in){
    galapagos_packet gp;
    size_t packet_size;
	short dest;
	short id;
   
    for(int j=0; j<iterations; j++){
        in->packet_read(mem, &packet_size, &dest, &id);
    }
}

void print_throughput(std::string test_name, std::string test_type, int size){
    std::chrono::duration<double> diff = end-start;
    double diff_s = diff.count();
    double throughput = (size*NUM_ITERATIONS*sizeof(ap_uint<64>)*8/1E6)/diff_s; // Mb/s

    std::cout << "timing," << test_name << "," << test_type << "," << size << "," << throughput << std::endl;
}

void print_latency(std::string test_name, std::string test_type, int size, std::chrono::duration<double> diff){
    double diff_s = diff.count();
    double avg_latency = diff_s/NUM_ITERATIONS;

    std::cout << "timing," << test_name << "," << test_type << "," << size << "," << avg_latency << std::endl;
}

int msg_num = 11;
int msg_size[] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, MAX_BUFFER}; // should have msg_num elements
// int msg_size[] = {512};
int msg_reply = 2;

void kern_benchmark_reply_1(short id, galapagos_interface * in, galapagos_interface *out){
    cpu_set_t my_set;        /* Define your cpu_set bit mask. */
    CPU_ZERO(&my_set);       /* Initialize it all to 0, i.e. no CPUs selected. */
    CPU_SET(12, &my_set);     /* set the bit that represents core 7. */
    sched_setaffinity(0, sizeof(cpu_set_t), &my_set);

    char* mem = (char *)malloc(MAX_BUFFER*sizeof(ap_uint<64>));
    
    for(int k = 0; k < 2; k++){
        for(int i = 0; i < msg_num; i++){
            // receive_packet_perf(NUM_ITERATIONS, msg_size[i], in);
            // generate_flit(NUM_ITERATIONS, msg_reply, id, id-1, out);
            for(int j = 0; j < NUM_ITERATIONS; j++){
                // auto timer = start_timer();
                receive_packet_perf(1, msg_size[i], in);
                // print_time(timer, "kernel_receive_0");
                // timer = start_timer();
                generate_flit(1, msg_reply, id, id-1, out);
                // print_time(timer, "kernel_receive_1");
            }

            for(int j = 0; j < NUM_ITERATIONS; j++){
                receive_packet_perf(1, msg_size[i], in);
                generate_flit(1, msg_reply, id, id-1, out);
            }
        }
    }

    for(int k = 0; k < 2; k++){
        for(int i = 0; i < msg_num; i++){
            // receive_packet_mem_perf(mem, NUM_ITERATIONS, msg_size[i], in);
            // generate_flit(NUM_ITERATIONS, msg_reply, id, id-1, out);
            for(int j = 0; j < NUM_ITERATIONS; j++){
                receive_packet_mem_perf(mem, 1, msg_size[i], in);
                generate_flit(1, msg_reply, id, id-1, out);
            }

            for(int j = 0; j < NUM_ITERATIONS; j++){
                receive_packet_mem_perf(mem, 1, msg_size[i], in);
                generate_flit(1, msg_reply, id, id-1, out);
            }
        }
    }
}

int main(){

    std::vector<std::string> kern_info_table;
    kern_info_table.push_back(std::string("10.1.3.101")); //kern 0 node address is 10.1.2.155
    kern_info_table.push_back(std::string("10.1.3.102")); //kern 1 node address is 10.1.2.155

    std::vector < galapagos::external_driver<ap_uint<64> > * > ext_drivers;
    galapagos::net::tcp <ap_uint<64> > my_tcp(
        7, // TCP port
        kern_info_table,
        "10.1.3.102"
    );
    ext_drivers.push_back(&my_tcp);

    #if LOG_LEVEL > 0
    galapagos::node<ap_uint <64> > node0(kern_info_table, std::string("10.1.3.102"), ext_drivers, my_logger);
    #else
    galapagos::node<ap_uint <64> > node0(kern_info_table, std::string("10.1.3.102"), ext_drivers);
    #endif
    // node0.add_kernel(0, kern_benchmark_reply_0);
    node0.add_kernel(1, kern_benchmark_reply_1);

    std::cout << std::endl << " ......................." << "test" << "......................." << std::endl;

    node0.start();
    node0.end();
 
    // std::chrono::duration<double> diff = end-start;
    // std::cout << std::endl << " ......................." << Catch::getResultCapture().getCurrentTestName() << "......................." << std::endl;
    // std::cout << "RUNTIME:"  <<  diff.count() << " s" << std::endl;
    // std::cout << "TRANSFER_RATE:"  <<  ((MAX_BUFFER*NUM_ITERATIONS*sizeof(ap_uint<64>))/diff.count()/(1000*1000/8)) << " Mb/s" << std::endl;

    std::cout << "DONE" << std::endl;
}