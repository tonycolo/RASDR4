# RASDR4
Radio Astronomy software for RASDR4/LimeSDR

The purpose of this software is to provide support for common radio astronomy tasks through both visualization, data processing, and collection writing to files.
At the moment, this consists of concurrent dual channel RX streaming I & Q, FFT conversion, power spectrum graphing, and writing FFT frames to files.
Most common streaming parameters, such as center frequency, effective sampling rate, decimation, low pass filter bandwidth, gain (LNA, TIA, PGA), and radio front end band
are supported.

External libraries used are:
 - FFTW3
 - Poco 
 - RTAudio (experimental)
 - LimeSuite

