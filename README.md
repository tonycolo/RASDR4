# RASDR4
Radio Astronomy software for RASDR4/LimeSDR

The purpose of this software is to provide support for common radio astronomy tasks through both visualization, data processing, and collection writing to files.
At the moment, this consists of concurrent dual channel RX streaming I & Q, FFT conversion, power spectrum graphing, and writing FFT frames to files.
Most common streaming parameters, such as center frequency, effective sampling rate, decimation, low pass filter bandwidth, gain (LNA, TIA, PGA), and radio front end band
are supported.

External libraries used are:
 - [FFTW3     Fast Fourier Transforms](http://www.fftw.org/)
 - [Poco      C++ Utilities](https://pocoproject.org/)
 - [RTAudio   Real time audio driver](https://www.music.mcgill.ca/~gary/rtaudio/)
 - [LimeSuite Driver/API for LimeSDR](https://github.com/tonycolo/RASDR4/tree/master)

## Build Instructions
* (WIP! Lots to do to enable Windows building)
1.  Download and build the above libraries
2.  Copy header files into include/
3.  Copy shared libaries into the primary project directory.
This entails the following:
  libLimeSuite.so.18.04-1
  libPocoFoundation.so.60
  libPocoJSON.so.60
  libPocoUtil.so.60
  libPocoXML.so.60
  librtaudio.so

4.  You might need to adjust CMakelists.txt to resolve path issues.
5.  cd into the main directory
6.  mkdir build
7.  cd build
8.  cmake ../
9.  make

TO DO:  automatically download, build, and configure dependencies [like this](https://github.com/bvacaliuc/fftw-calc/blob/master/dependencies/fftw/CMakeLists.txt)

See the [running instructions](https://tinyurl.com/yadlzbhp) for using the application, configuring USB, troubleshooting, and so forth.

## Development
The core class file with main is rasdr-fftw.cpp

