#8051 iss
把data.h, data.cpp加進8051 iss
把原本的top.h換成此份
重新編譯8051_iss
#IAR embedded workbench
以IAR ew編譯software_v2.c成<name>.hex
再用hex2byte轉成<name>.t
#run
./8051_iss <name>.t data.dmp FFT_simulation/golden.txt
其中data.dmp為FFT之輸入，為HEX file形式，可用支援hex格式的
文字編輯器直接改成想要的測資。