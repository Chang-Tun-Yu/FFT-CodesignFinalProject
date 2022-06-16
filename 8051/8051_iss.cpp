#define _CRT_SECURE_NO_WARNINGS

#include <systemc.h>
#include <fstream>
#include <vector>
#include <complex>
#include <math.h>
#include <iomanip>
//#include <conio.h>
//#include <ncurses.h>
#define clockcycle      10   //10ns=100MHz
#define clockcycle_8051  4 //
#include "top.h"

namespace Configs {
	const int mode = 0; // 1 for N cycle, 0 for one cycle
	const bool debug = 0;
}

int sc_main(int argc, char *argv[])
{
	sc_core::sc_report_handler::set_actions("/IEEE_Std_1666/deprecated",
		sc_core::SC_DO_NOTHING);

    char *filename;
	bool dolog;
	std::vector<std::complex<double> > golden;
	std::vector<std::complex<double> > asic_ans;
	std::vector<std::complex<double> > errs;
	int load_latency, output_latency, overall_latency;

	if (argc<2) {
	    printf("Usage:8051_iss filename imagename\n");
	    return(1);
	}
	if (argc<3) {
	    printf("Usage:8051_iss filename imagename\n");
	    return(1);
	}
	filename=argv[1];
        sc_clock                clk("CLOCK",clockcycle,SC_NS,0.5,1,SC_NS);
		sc_clock                clk_8051("CLOCK_8051",clockcycle_8051, SC_NS,0.5,1,SC_NS);
        sc_signal<sc_uint<1> >  reset;
        sc_signal<sc_uint<1> >  poff;
		
        top top("top");
        top.clk(clk);
		top.clk_8051(clk_8051);
        top.reset(reset);
        top.poff(poff);
		

        if (!top.core->loadrom(filename)) {
            printf("%s file not found\n",filename);
            return(1);
        }
		filename=argv[2];
		{
			FILE *FR;
			unsigned char s;
			int i;
			FR=fopen(filename,"rb");
			if (FR==NULL) {printf("%s file not found\n",filename);return(1);}
			for(i=0;i<8*8;i++)
			{
				s=fgetc(FR);
		        //printf(" %d xram data: %x ",i, s);
				top.xrambig->mem[(0x3000)+i]=s;
			}
			fclose (FR);
		}
		filename=argv[3];
		double rr, ii;
		std::fstream f;
		f.open(filename);
		while (f >> rr >> ii)
		{
			std::complex<double> tmp(rr, ii);
			golden.push_back(tmp);
		}
		f.close();
		// for (int i = 0; i < 64; i++)
		// 	std::cout << golden.at(i).real() << std::endl;

        ///////////////////////////////////Start Test
		VARS_8051   olds, news;
		int t,k;
		unsigned int out_r;
		unsigned int out_i;
		double ans_r;
		double ans_i;
		int out_index = 0;
		int out_cnt = 0;
		int load_start = 0;
		int output_start = 0;
		int output_end = 0;
		sc_trace_file* Tf;
		Tf = sc_create_vcd_trace_file("waveform");
		sc_trace(Tf, clk_8051, "clk_8051");
		sc_trace(Tf, clk, "clk");

		sc_trace(Tf, top.port0o, "port0o");
		sc_trace(Tf, top.port3o, "port3o");
		sc_trace(Tf, top.port2i, "port2i");
		sc_trace(Tf, top.port1i, "port1i");
	    sc_trace(Tf, top.port3i, "port3i");
		reset.write(1);

		for(t=0;t<10;t++) {
			sc_start(clockcycle,SC_NS);
		}
		reset.write(0);
		top.core->save_vars(&olds);

		for (; t < 3000; t++) {
			int p0o = top.port0o.read();  //A 
			int p0i = top.port0i.read();  //D0
			int p1 = top.port1i.read();   //D1
			int p2 = top.port2i.read();   //C 
			int p3i = top.port3i.read();  
			int p3o = top.port3o.read();  //B 
			// if (t % 10 == 0 || t > 700) {
			// 	printf("\n");
			// 	printf("Time:%d, P0o=%d, P0i=%d, P1=%d, P2=%d, P3i=%d, P3o=%d\n\n", t * clockcycle, p0o, p0i, p1, p2, p3i, p3o);
			// }
			// else {
			// 	printf(". ");
			// }
			if (p0o == 0 && load_start == 0) {
				load_start = t;
			}
			if (p2 == 1) { // load finish
				std::cout << "[Load finish]: Time Used = " << (t - load_start)*clockcycle << " ns"<< std::endl;
				load_latency = (t - load_start)*clockcycle;
				std::cout << "============================ Validation ============================" << std::endl;
			}
			if ((p2 / 128 == 1) && output_start == 0) {
				output_start = t;
			}
			if (p2 / 128 == 1) {
				if (out_cnt == 0) {
					out_r = 256*p0i + p1;
					out_cnt += 1;
				}
				else if (out_cnt == 1) {
					out_r *= (256*256);
					out_r += 256*p0i + p1;
					out_cnt += 1;
				}
				else if (out_cnt == 2) {
					out_i = 256*p0i + p1;
					out_cnt += 1;
				}
				else {
					out_i *= (256*256);
					out_i += 256*p0i + p1;
					// convert to double
					if (out_r / (1 << 31) == 1) { // negative
						// std::cout << "neg" << std::endl;
						ans_r = (static_cast<double>(out_r) - 2*static_cast<double>(static_cast<unsigned long long>(1) << 31)) / static_cast<double>(1 << 16);
					}
					else {
						ans_r = static_cast<double>(out_r) / static_cast<double>(1 << 16);
					}
					if (out_i / (1 << 31) == 1) { // negative
						// std::cout << "neg" << std::endl;
						ans_i = (static_cast<double>(out_i) - 2*static_cast<double>(static_cast<unsigned long long>(1) << 31)) / static_cast<double>(1 << 16);
					}
					else {
						ans_i = static_cast<double>(out_i) / static_cast<double>(1 << 16);
					}
					std::cout << "[Result] " << std::setw(2)<< out_index << 
					": my (" << std::setw(10) <<ans_r << ", " << std::setw(10) <<ans_i << "), golden (" << 
					std::setw(10) <<golden[out_index].real() << ", " << std::setw(10) <<golden[out_index].imag() << ")"<< std::endl;

					std::complex<double> tmp(ans_r, ans_i);
					asic_ans.push_back(tmp);
					// std::cout  << std::endl;
					// next data setting
					out_cnt = 0;
					out_index += 1;
					out_r = 0;
					out_i = 0;
					ans_r = 0;
					ans_i = 0;
					if (out_index == 64) {
						std::cout << "[Output finish]: Time Used = " << (t - output_start)*clockcycle << " ns"<< std::endl;
						output_latency = (t - output_start)*clockcycle;
						overall_latency = t*clockcycle;
					}
				}
			}
			sc_start(clockcycle,SC_NS);
		}
		// compute SNR
		for (int i = 0; i < 64; i++)
			errs.push_back(golden.at(i) - asic_ans.at(i));
		double sig = 0; 
		double noise = 0;
		for (int i = 0; i < 64; i++)
			sig += std::abs(asic_ans.at(i)) * std::abs(asic_ans.at(i));
		sig = sig / (double)64;
		for (int i = 0; i < 64; i++)
			noise += std::abs(errs.at(i)) * std::abs(errs.at(i));
		noise = noise / (double)64;
		double SNR;
		SNR = 10*log10(sig/noise);
		
		int compute_latency;
		if (Configs::mode) { // N-CYCLE
			compute_latency = 540;
		}
		else {
			compute_latency = 10;
		}
		std::cout <<  std::endl;
		std::cout << "============================================================" << std::endl;
		std::cout << "                === Simulation Result ===                   " << std::endl;
		std::cout << "               Load Latency: " << std::setw(9) << load_latency << " (ns)"<< std::endl;
		std::cout << "               Compute Latency: " << std::setw(6) << compute_latency << " (ns)"<<std::endl;
		std::cout << "               Output Latency: " << std::setw(7) << output_latency << " (ns)"<<std::endl;
		std::cout << "               Overall Latency: " << std::setw(6) << overall_latency << " (ns)"<<std::endl;
		std::cout << "               SNR of 2 FFT:  " << std::setw(8) << SNR << " (db)" << std::endl;
		std::cout << "============================================================" << std::endl;
		std::cout <<  std::endl;
					
		sc_close_vcd_trace_file(Tf);
		system("pause");
        return(0);
}
