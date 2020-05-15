#pragma once

// https://stackoverflow.com/a/39014279
class my_barrier
{

 public:
    my_barrier(int count)
     : thread_count(count)
     , counter(0)
     , waiting(0)
    {}

    void wait()
    {
        //fence mechanism
        std::unique_lock<std::mutex> lk(m);
        ++counter;
        ++waiting;
        cv.wait(lk, [&]{return counter >= thread_count;});
        cv.notify_one();
        --waiting;
        if(waiting == 0)
        {
           //reset barrier
           counter = 0;
        }
        lk.unlock();
    }

 private:
      std::mutex m;
      std::condition_variable cv;
      int counter;
      int waiting;
      int thread_count;
};

//Test from flit to flit
TEST_CASE( "BENCHMARK:PERF" ) {

   
    std::vector<std::string> kern_info_table;
    kern_info_table.push_back(std::string("10.1.2.155")); //kern 0 node address is 10.1.2.155
    kern_info_table.push_back(std::string("10.1.2.155")); //kern 1 node address is 10.1.2.155


    #if LOG_LEVEL > 0
    galapagos::node<ap_uint <64> > node0(kern_info_table, std::string("10.1.2.155"), std::vector<galapagos::external_driver <ap_uint<64> > * >(), my_logger);
    #else
    galapagos::node<ap_uint <64> > node0(kern_info_table, std::string("10.1.2.155"), std::vector<galapagos::external_driver <ap_uint<64> > * >());
    #endif
    node0.add_kernel(0, kern_benchmark_0);
    node0.add_kernel(1, kern_benchmark_1);

    std::cout << std::endl << " ......................." << Catch::getResultCapture().getCurrentTestName() << "......................." << std::endl;

    node0.start();
    node0.end();
 
    // std::chrono::duration<double> diff = end-start;
    // std::cout << std::endl << " ......................." << Catch::getResultCapture().getCurrentTestName() << "......................." << std::endl;
    // std::cout << "RUNTIME:"  <<  diff.count() << " s" << std::endl;
    // std::cout << "TRANSFER_RATE:"  <<  ((MAX_BUFFER*NUM_ITERATIONS*sizeof(ap_uint<64>))/diff.count()/(1000*1000/8)) << " Mb/s" << std::endl;

    std::cout << "DONE" << std::endl;

}