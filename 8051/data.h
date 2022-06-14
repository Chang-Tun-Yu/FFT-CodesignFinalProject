#ifndef	__DATA_H
#define __DATA_H
#include "systemc.h"
#include <stdio.h>

SC_MODULE(DATA_asic)
{
	sc_in_clk			    clk;
	sc_in<sc_uint<1> >		reset;
	sc_in<sc_uint<8> >      xram_data;  // data from Xram
	sc_in<sc_uint<8> >      ctrl;  		// P3o: control signal to data transmission. 0:stop, 1:start
	sc_out<sc_uint<8> >     data;  		// P3i: return data from Xram
	sc_signal<bool> rst;
	
	SC_CTOR(DATA_asic)
	{
		rst = true;
		SC_CTHREAD(GET, clk.neg());
	}
	void GET(){
	    int i;
	    if(rst){
		    i = 0;
	        rst = false;
		}
	    
	    while(1){
		    wait();
            if(ctrl.read() == 2){
			    data.write(xram_data);
                i++;
                //printf("%d th read: %hu\n", i, xram_data);
			}
		}
		    
	}
};
#endif