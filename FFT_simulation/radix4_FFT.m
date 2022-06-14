%% Set up a signal
a = 64;   % FFT Length
t = 0:1/a:(a-1)/a;
% signal
% s = cos(2*pi*6*t) + cos(2*pi*12*t)*j;
s1 = 0:31;
s2 = 48:63;
s3 = 96:111;
s = [s1,s2,s3];
subplot(211)
plot(t, real(s))
xlabel('time (sec)'); ylabel('Amplitude'); title('Test Signal - Real Component');
subplot(212)
plot(t, imag(s))
xlabel('time (sec)'); ylabel('Amplitude'); title('Test Signal - Imag Component');

%% Test FFT code
% run FFT
S = radix4FFT1_Float(s);
% permute the output into bit-reversed order
% [1, 9, 5, 13, 3, 11, 7, 15, 2, 10, 6, 14, 4, 12, 8, 16]
S = bitrevorder(S);

% Calculate FFT using MATLAB function
Y = fft(s);
fileID = fopen('golden.txt','w');
for i = 1:64
    fprintf(fileID, "%f %f\n", real(Y(i)), imag(Y(i)));
end
fclose(fileID);

% Compare accuracy of Radix-4 FFT to MATLAB's FFT
errs = double(S) - Y;
Sig = sum(abs(Y).^2) / a;
Noise = sum(abs(errs).^2) / a;
SNR = 10 * log10(Sig/Noise);
sprintf('SNR for 2 FFT methods is: %6.2f dB', SNR)

figure
subplot(411)
plot(1:a, real(Y))
subplot(412)
plot(1:a, imag(Y))
subplot(413)
plot(1:a, real(S))
subplot(414)
plot(1:a, imag(S))

%% FFT
function S = radix4FFT1_Float(s)
    % The length of the input signal should be a power of 4: 4, 16, 64, 256, etc.
    N = length(s); % length of the input signal
    M = log2(N)/2; % number of stages
 
    % Twiddle factors
    W = exp(-j*2*pi*(0:N-1)/N);
    % Variables for FFT results
    S = complex(zeros(1,N));
    sTemp = complex(zeros(1,N));

    % FFT algorithm
    % Calculate butterflies for the first M-1 stages
    sTemp = s;
    for stage = 0:M-2
        for n = 1:N/4
            S((1:4)+(n-1)*4) = radix4bfly(sTemp(n:N/4:end), floor((n-1)/(4^stage)) *(4^stage), 1, W);
        end
        sTemp = S;
    end
    
    % Calculate butterflies for the last stage
    for n = 1:N/4
        S((1:4)+(n-1)*4) = radix4bfly(sTemp(n:N/4:end), floor((n-1)/(4^stage)) * (4^stage), 0, W);
    end
    sTemp = S;
    
    % Rescale the final output
    S = S * 1;
   
end

function Z = radix4bfly(x, segment, stageFlag, W)
    % For the last stage of a radix-4 FFT all the ABCD multiplers are 1.
    % Use the stageFlag variable to indicate the last stage
    % stageFlag = 0 indicates last FFT stage, set to 1 otherwise

    % Initialize variables
    a = x(1);
    b = x(2);
    c = x(3);
    d = x(4);

    % Radix-4 Algorithm
    % A, B, C, D are the 1st, 3rd, 2nd, 4th outputs of the butterfly
    % this arrangement is based on bit-reversed order
    A = a + b + c + d;
    B = (a - b + c - d) * W(2*segment*stageFlag + 1);
    C = (a - b*j - c + d*j) * W(segment*stageFlag + 1);
    D = (a + b*j - c - d*j) * W(3*segment*stageFlag + 1);
    
    Z = [A B C D];

end


