#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<io51.h>

void init(void){
	P0=0;
	P1=0;
	P2=0;
	P3=0;
	DPH=48;
	DPL=0;
}

int main(void){
	unsigned char i = 0;
	init();
	while(1){
	    if(i<=128){
	        if(i%2==0){//get from xram
	            P3 = 2;
	            P0 = P3;
	            i += 1;
	            // P3 = 0;
	        }
	        else{//forward to FFT
	            P3 = 1;
	            i += 1;
	            DPL += 1;
	            // P3 = 0;
	        }
	    }
		else {
			P3 = 0;
		}
	}
}