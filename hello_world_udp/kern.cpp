#include <cstddef>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#include "../packet_size.h"
#include "../galapagos_interface.hpp"
#include "kern.hpp"


void kern_send(short id, 
        galapagos_interface * in, 
        galapagos_interface  * out
        )
{


#pragma HLS INTERFACE axis register both port=out
#pragma HLS INTERFACE axis register both port=in
    

    for (int i=0; i<N; i++){
        T data_array[F];
        for(int j=0; j<F; j++){
            for(int k=0; k<PACKET_DATA_LENGTH/8; k++){
                data_array[j]((k+1)*8-1,k*8) = k; //packs data to lower half of flit
            }
            std::cout<< "Sending Flit " << std::hex << data_array[j] << std::dec << std::endl;
        }
	    out->packet_write((char *)data_array, F, id+1, id);
    }

    for(int i=0; i<N; i++){
        size_t size;
        short dest, id;
        T * data_array = (T *)in->packet_read(&size, &dest, &id);
        
        for(int i=0; i<F; i++){
            std::cout<< "Loopback Flit " << std::hex << data_array[i] << std::dec << std::endl;
        }

    }

}


void kern_loopback(short id, 
        galapagos_interface * in, 
        galapagos_interface  * out
        )
{


#pragma HLS INTERFACE axis register both port=out
#pragma HLS INTERFACE axis register both port=in
   

    for(int i=0; i<N; i++){
        size_t size;
        short dest, src_id;
        T * data_array = (T *)in->packet_read(&size, &dest, &id);

        for(int j=0; j<F; j++){
            std::cout<< "Received Flit " << std::hex << data_array[j] << std::dec << std::endl;
            data_array[j]++;
        }
	    
        out->packet_write((char *)data_array, F, id, id-1);

        
    }

}
