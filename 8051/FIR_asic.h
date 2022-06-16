#ifndef	__ASIC_H
#define __ASIC_H

// #define DEBUG 
//#define N_CYCLE 
#include "systemc.h"
#include <stdio.h>
#include <cmath>
#include <complex>
#include <bitset>
#include <assert.h>

#define BW 6

SC_MODULE(FIR_asic)
{
	sc_in_clk			    clk;
	sc_in<sc_uint<1> >		reset;
	sc_in<sc_uint<8> >      A;  	// P0: input random A
	sc_in<sc_uint<8> >      B;  	// P3: control signal to FIR
	sc_out<sc_uint<8> >     C;  	// P2: has_reset
	sc_out<sc_uint<8> >     D0;  	
	sc_out<sc_uint<8> >     D1;  	
	sc_signal<bool> rst;
	sc_signal<bool> prev_valid;

	SC_CTOR(FIR_asic)
	{
		rst = true;
		prev_valid = false;
		#ifdef N_CYCLE
		SC_CTHREAD(FIR_N, clk.neg());
		#else
		SC_CTHREAD(FIR_1, clk.neg());
		#endif
		
	}
	
	/* 1 cycle */
	void FIR_1() {
		using namespace std::complex_literals;

		if (rst)
		{
			C.write(0x00);
			D0.write(0x00);
			D1.write(0x00);
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
		sc_int<32> result_r;
		sc_int<32> result_i;
		bool load_finish = false;
		bool cal_finish = false;

		std::complex<double> a, b, c, d;
		unsigned int segment;
		bool discard = true;
		int out_cnt = 0;

		C.write(0);
		while (1) {
			wait();
			// std::cout << "[FFT]" << A << " " << B << " " << C << " "<< D << endl;
			if (B.read() == 1 && !prev_valid) {
				// discard the first 0
				// if (discard) {
				// 	discard = false;
				// }
				// else {
					s_t[index++] = std::complex<double>(A.read(), 0);
					std::cout << "[LOAD] " <<"s_t[" << index - 1 << "] = " << s_t[index - 1] << std::endl;
					#ifdef DEBUG
					if (index - 1 <= 31)
						assert(index-1 == s_t[index - 1].real());
					else if (index -1 <= 47)
						assert(index-1 == s_t[index - 1].real() - 16);
					else 
						assert(index-1 == s_t[index - 1].real() - 48);
					#endif
					if (index >= LEN) {
						load_finish = true;
					}			
				// }
			}
			prev_valid = (B.read() == 1);
				
			if (cal_finish) {
				if (out_cnt == 0) {
					// std::cout << "[WRITE OUT]" << write_index << " " << S_f[write_index] << std::endl;
					result_r = static_cast<sc_int<32> >(S_f[write_index].real()*(1 << 16));
					// use .range(31, 24) to select
					C.write(128+write_index);
					D0.write(static_cast<sc_uint<8> >(result_r.range(31, 24)));
					D1.write(static_cast<sc_uint<8> >(result_r.range(23, 16)));
					out_cnt += 1;
				}
				else if (out_cnt == 1) {
					C.write(128+write_index);
					D0.write(static_cast<sc_uint<8> >(result_r.range(15, 8)));
					D1.write(static_cast<sc_uint<8> >(result_r.range(7, 0)));
					out_cnt += 1;
				}
				else if (out_cnt == 2) {
					result_i = static_cast<sc_int<32> >(S_f[write_index].imag()*(1 << 16));
					C.write(128+64+write_index);
					D0.write(static_cast<sc_uint<8> >(result_i.range(31, 24)));
					D1.write(static_cast<sc_uint<8> >(result_i.range(23, 16)));
					out_cnt += 1;
				}
				else if (out_cnt == 3) {
					C.write(128+64+write_index);
					D0.write(static_cast<sc_uint<8> >(result_i.range(15, 8)));
					D1.write(static_cast<sc_uint<8> >(result_i.range(7, 0)));
					out_cnt = 0;
					write_index += 1;
				}
				else { // == 8
					C.write(0);
					D0.write(0);
					out_cnt = 0;
				}
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
				C.write(1);
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
		using namespace std::complex_literals;

		if (rst)
		{
			C.write(0x00);
			D0.write(0x00);
			D1.write(0x00);
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
		sc_int<32> result_r;
		sc_int<32> result_i;
		bool load_finish = false;
		bool cal_finish = false;

		std::complex<double> a, b, c, d;
		unsigned int segment;
		bool discard = true;
		int out_cnt = 0;

		C.write(0);
		bool do_init = true;
		bool do_fft1 = false;
		bool do_data_trans = false;
		bool do_fft2 = false;
		bool do_bit_rev = false;
		int stage = 0;
		int n = 0;
		int m = 0;
		while (1) {
			wait();
			// std::cout << "[FFT]" << A << " " << B << " " << C << " "<< D << endl;
			if (B.read() == 1 && !prev_valid) {
				// discard the first 0
				// if (discard) {
				// 	discard = false;
				// }
				// else {
					s_t[index++] = std::complex<double>(A.read(), 0);
					// std::cout << "[LOAD] " <<"s_t[" << index - 1 << "] = " << s_t[index - 1] << std::endl;
					#ifdef DEBUG
					if (index - 1 <= 31)
						assert(index-1 == s_t[index - 1].real());
					else if (index -1 <= 47)
						assert(index-1 == s_t[index - 1].real() - 16);
					else 
						assert(index-1 == s_t[index - 1].real() - 48);
					#endif
					if (index >= LEN) {
						load_finish = true;
					}			
				// }
			}
			prev_valid = (B.read() == 1);
			// std::cout << "stage: " << stage << ", m: " << m << ", n: " << n << std::endl;

			if (cal_finish) {
				if (out_cnt == 0) {
					// std::cout << "[WRITE OUT]" << write_index << " " << S_f[write_index] << std::endl;
					result_r = static_cast<sc_int<32> >(S_f[write_index].real()*(1 << 16));
					// use .range(31, 24) to select
					C.write(128+write_index);
					D0.write(static_cast<sc_uint<8> >(result_r.range(31, 24)));
					D1.write(static_cast<sc_uint<8> >(result_r.range(23, 16)));
					out_cnt += 1;
				}
				else if (out_cnt == 1) {
					C.write(128+write_index);
					D0.write(static_cast<sc_uint<8> >(result_r.range(15, 8)));
					D1.write(static_cast<sc_uint<8> >(result_r.range(7, 0)));
					out_cnt += 1;
				}
				else if (out_cnt == 2) {
					result_i = static_cast<sc_int<32> >(S_f[write_index].imag()*(1 << 16));
					C.write(128+64+write_index);
					D0.write(static_cast<sc_uint<8> >(result_i.range(31, 24)));
					D1.write(static_cast<sc_uint<8> >(result_i.range(23, 16)));
					out_cnt += 1;
				}
				else if (out_cnt == 3) {
					C.write(128+64+write_index);
					D0.write(static_cast<sc_uint<8> >(result_i.range(15, 8)));
					D1.write(static_cast<sc_uint<8> >(result_i.range(7, 0)));
					out_cnt = 0;
					write_index += 1;
				}
				else { // == 8
					C.write(0);
					D0.write(0);
					out_cnt = 0;
				}
				if (write_index >= LEN) {
					load_finish = false;
					cal_finish = false;
					index = 0;
					write_index = 0;
				}
			}
			else if (load_finish) {
				if (do_init) {
					// std::cout << "init" << std::endl;
					for (int k = 0; k < LEN; ++k) {
						S_f[k] = 0;
						sTemp[k] = s_t[k];
					}
				}
				// Calculate butterflies for the first M-1 stages
				else if (do_fft1) {
					// std::cout << "fft1" << std::endl;
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
				else if (do_data_trans) {
					// std::cout << "data_trans" << std::endl;
					for (int k = 0; k < LEN; ++k) {
						sTemp[k] = S_f[k];
					}
				}
				else if (do_fft2) {
					// std::cout << "fft2" << std::endl;
					a = sTemp[m];
					b = sTemp[m + LEN / 4];
					c = sTemp[m + LEN / 2];
					d = sTemp[m + (3 * LEN / 4)];
					S_f[0 + m * 4] = a + b + c + d;
					S_f[1 + m * 4] = (a - b + c - d);
					S_f[2 + m * 4] = (a - b * 1i - c + d * 1i);
					S_f[3 + m * 4] = (a + b * 1i - c - d * 1i);
				}
				else if (do_bit_rev) {
					// std::cout << "bit_rev" << std::endl;
					for (int k = 0; k < LEN; ++k) {
						sTemp[k] = S_f[k];
					}
					for (int k = 0; k < LEN; ++k) {
						std::bitset<BW> n(k);
						S_f[reverseBits(n)] = sTemp[k];
					}
				}
				else {
					// std::cout << "error" << std::endl;
				}

				// new index generation
				if (do_init) {
					do_init = false;
					do_fft1 = true;
					C.write(1);
				}
				else if (do_fft1 || do_data_trans) {
					C.write(0);
					if (n < LEN / 4 - 1) {
						n += 1;
						do_fft1 = true;
					}
					else {
						if (!do_data_trans) {
							do_fft1 = false;
							do_data_trans = true;
						}
						else {
							stage += 1;
							if (stage <= num_stage-2) {
								do_fft1 = true;
								do_data_trans = false;
								n = 0;
							}
							else {
								do_fft1 = false;
								do_data_trans = false;
								do_fft2 = true;
							}
						}	
					}
				}
				else if (do_fft2) {
					C.write(0);
					if (m < LEN / 4 - 1) {
						m += 1;
						do_fft2 = true;
					}
					else {
						do_fft2 = false;
						do_bit_rev = true;
					}
				}
				else if (do_bit_rev) {
					C.write(0);
					cal_finish = true;
				}
				
				// for (int stage = 0; stage <= num_stage-2; ++stage) {
				// 	for (int n = 0; n < LEN / 4; ++n) {
						
				// 	}
					
				// }
				// Calculate butterflies for the last stage
				// for (int n = 0; n < LEN / 4; ++n) {
					
				// }
				// bit reversed order
				
				
				
			}
			else
				C.write(0);
		}
	}
};

#endif
