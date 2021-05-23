
#include <string>
#include <math.h>
#include <thread>
#include <chrono>
#include <boost/program_options.hpp>

#include "../galapagos_node.hpp"
#include "../galapagos_net_udp.hpp"
#include "kern.hpp"


#define SEND 0
#define LOOPBACK 1
#define BOTH 2

#define GALAPAGOS_PORT 7
#define IP_ADDR_1 "10.1.2.155"
#define IP_ADDR_2 "10.1.2.156"



int main(int argc, const char** argv){



    std::vector <std::string> kern_info;

// IP ADDRESSES
// Two Kernels: DEST 0 is at IP_ADDR_1
// DEST 1 is at IP_ADDR_2
    std::string source_ip_str;
    std::string dest_ip_str;
    source_ip_str=IP_ADDR_1;
    dest_ip_str=IP_ADDR_2;

    kern_info.push_back(source_ip_str);
    kern_info.push_back(dest_ip_str);

    std::vector < galapagos::external_driver<T> * > ext_drivers;
    galapagos::net::udp <T> my_udp(
                GALAPAGOS_PORT, 
                kern_info, 
                source_ip_str 
            );
    ext_drivers.push_back(&my_udp);
    std::unique_ptr<galapagos::node <T> > node_ptr;
    node_ptr = std::make_unique<galapagos::node <T> >(kern_info, source_ip_str, ext_drivers);

//Adding Kernels    
    node_ptr->add_kernel(0, kern_send);
    node_ptr->add_kernel(1, kern_loopback);

    node_ptr->start();
    node_ptr->end();

}
