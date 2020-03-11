
#include <string>
#include <math.h>
#include <thread>
#include <chrono>
#include <boost/program_options.hpp>

#include "../galapagos_node.hpp"
#include "kern.hpp"


#define SEND 0
#define LOOPBACK 1
#define BOTH 2

#define IP_ADDR_1 "10.1.2.155"
#define IP_ADDR_2 "10.1.2.156"


int get_node_type(int argc, const char ** argv){
    
    std::string type;

    try{
        
        boost::program_options::options_description desc{"Options"};
        desc.add_options()
                ("help,h", "Help screen")
                ("type", boost::program_options::value<std::string>()->default_value(std::string("both")), "type");
        boost::program_options::variables_map vm;
        store(parse_command_line(argc, argv, desc), vm);
        notify(vm);
        if (vm.count("help"))
            std::cout << desc << std::endl;
        if(vm.count("send"))
            type = vm["type"].as<std::string>();



    }
    catch (const boost::program_options::error &ex)
    {
        std::cerr << ex.what() << '\n';              
    }


    int ret_type;
    if(type == "send")
        ret_type = SEND;
    else if(type == "loopback")
        ret_type = LOOPBACK;
    else
        ret_type = BOTH;

    return ret_type;
}


int main(int argc, const char** argv){

   

    int node_type = get_node_type(argc, argv);


    std::vector <std::string> kern_info;

// IP ADDRESSES
// Two Kernels: DEST 0 is at IP_ADDR_1
// DEST 1 is at IP_ADDR_2
    std::string source_ip_str;
    std::string dest_ip_str;
    if(node_type == BOTH){
        source_ip_str=IP_ADDR_1;
        dest_ip_str=IP_ADDR_1;
    }
    else{
        source_ip_str=IP_ADDR_1;
        dest_ip_str=IP_ADDR_2;
    }

    kern_info.push_back(source_ip_str);
    kern_info.push_back(dest_ip_str);

    std::vector < galapagos::external_driver<T> * > ext_drivers;

    std::unique_ptr<galapagos::node <T> > node_ptr;
#if LOG_LEVEL > 0    
    std::shared_ptr<spdlog::logger> my_logger;
    my_logger = spdlog::basic_logger_mt("basic_logger", "hello_world_log.txt"); 
#endif
    if(node_type == SEND)
#if LOG_LEVEL > 0
        node_ptr = std::make_unique<galapagos::node <T> >(kern_info, source_ip_str, ext_drivers, my_logger);
#else
        node_ptr = std::make_unique<galapagos::node <T> >(kern_info, source_ip_str, ext_drivers);
#endif
    else if(node_type == LOOPBACK)
#if LOG_LEVEL > 0
        node_ptr = std::make_unique<galapagos::node <T> >(kern_info, dest_ip_str, ext_drivers, my_logger);
#else
        node_ptr = std::make_unique<galapagos::node <T> >(kern_info, dest_ip_str, ext_drivers);
#endif
    else
#if LOG_LEVEL > 0
        node_ptr = std::make_unique<galapagos::node <T> >(kern_info, source_ip_str, ext_drivers, my_logger);
#else
        node_ptr = std::make_unique<galapagos::node <T> >(kern_info, source_ip_str, ext_drivers);
#endif

//Adding Kernels    
    node_ptr->add_kernel(0, kern_send);
    node_ptr->add_kernel(1, kern_loopback);

    node_ptr->start();
    node_ptr->end();

}
