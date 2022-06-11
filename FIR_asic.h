#ifndef	__ASIC_H
#define __ASIC_H
#include "systemc.h"
#include <stdio.h>
#include <cmath>
#include <complex>
#include <bitset>

#define BW 6

SC_MODULE(FIR_asic)
{
	sc_in_clk			    clk;
	sc_in<sc_uint<1> >		reset;
	sc_in<sc_uint<8> >      A;  	// P0: input random A
	sc_in<sc_uint<8> >      B;  	// P3: control signal to FIR
	sc_out<sc_uint<8> >     C;  	// P2: has_reset
	sc_out<sc_uint<8> >     D;  	// P1: 8051 forwards result to Port2
	sc_signal<bool> rst;

	SC_CTOR(FIR_asic)
	{
		rst = true;
		SC_CTHREAD(FIR_1, clk.neg());
		// SC_CTHREAD(FIR_N, clk.neg());
	}
	
	/* 1 cycle */
	void FIR_1() {
		using namespace std::complex_literals;

		if (rst)
		{
			C.write(0x00);
			D.write(0x00);
			rst = false;
		}
		
		const int LEN = 64, num_stage = 3;
		const double _2pi = std::acos(-1) * 2;
		// Twiddle factors
		std::complex<double> W[LEN];
		for (int k = 0; k < LEN; ++k) {
			W[k] = std::exp(-1i * _2pi * double(k) / double(LEN));
		}
		// Variables for FFT results
		std::complex<double> S_f[LEN];
		std::complex<double> sTemp[LEN];

		// input signal
		std::complex<double> s_t[LEN];
		
		sc_int<8> index = 0;
		sc_int<8> write_index = 0;
		bool load_finish = false;
		bool cal_finish = false;

		std::complex<double> a, b, c, d;
		unsigned int segment;
		bool discard = true;
		
		C.write(0);
		while (1) {
			wait();
			if (B.read() == 1) {
				// discard the first 0
				if (discard) {
					discard = false;
				}
				else {
					s_t[index++] = std::complex<double>(A.read(), 0);
					std::cout << "s_t[" << index - 1 << "] = " << s_t[index - 1] << std::endl;
					if (index >= LEN)
						load_finish = true;
				}
			}
				
			if (cal_finish) {
				std::cout << write_index << " " << S_f[write_index] << std::endl;
				D.write(S_f[write_index++].real());
				C.write(1);
				if (write_index >= LEN) {
					load_finish = false;
					cal_finish = false;
					index = 0;
					write_index = 0;
				}
			}
			else if (load_finish) {
				for (int k = 0; k < LEN; ++k) {
					S_f[k] = 0;
					sTemp[k] = s_t[k];
				}
				// Calculate butterflies for the first M-1 stages
				for (int stage = 0; stage <= num_stage-2; ++stage) {
					for (int n = 0; n < LEN / 4; ++n) {
						a = sTemp[n];
						b = sTemp[n + LEN / 4];
						c = sTemp[n + LEN / 2];
						d = sTemp[n + (3 * LEN / 4)];
						segment = floor(n / pow(4, stage)) * pow(4, stage);
						S_f[0 + n * 4] = a + b + c + d;
						S_f[1 + n * 4] = (a - b + c - d) * W[2 * segment];
						S_f[2 + n * 4] = (a - b * 1i - c + d * 1i) * W[segment];
						S_f[3 + n * 4] = (a + b * 1i - c - d * 1i) * W[3 * segment];
					}
					for (int k = 0; k < LEN; ++k) {
						sTemp[k] = S_f[k];
					}
				}
				// Calculate butterflies for the last stage
				for (int n = 0; n < LEN / 4; ++n) {
					a = sTemp[n];
					b = sTemp[n + LEN / 4];
					c = sTemp[n + LEN / 2];
					d = sTemp[n + (3 * LEN / 4)];
					S_f[0 + n * 4] = a + b + c + d;
					S_f[1 + n * 4] = (a - b + c - d);
					S_f[2 + n * 4] = (a - b * 1i - c + d * 1i);
					S_f[3 + n * 4] = (a + b * 1i - c - d * 1i);
				}
				// bit reversed order
				for (int k = 0; k < LEN; ++k) {
					sTemp[k] = S_f[k];
				}
				for (int k = 0; k < LEN; ++k) {
					std::bitset<BW> n(k);
					S_f[reverseBits(n)] = sTemp[k];
				}
				C.write(0);
				cal_finish = true;
			}
			else
				C.write(0);
		}
	}

	unsigned long reverseBits(std::bitset<BW> n)
	{
		std::bitset<BW> rev = 0;

		for (int k = BW - 1; k >= 0; k--) {
			rev |= (n & std::bitset<BW>(1)) << k;
			n >>= 1;
		}

		return rev.to_ulong();
	}
	
	/* N cycle */
	void FIR_N() {
		if (rst)
		{
			C.write(0x00);
			D.write(0x00);
			rst = false;
		}

		C.write(0);
		while (1) {
			wait();
		}
	}
};

#endif
