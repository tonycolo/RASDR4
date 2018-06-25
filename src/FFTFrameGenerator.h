//
// Created by astro on 1/28/18.
//

#ifndef RASDR_FFTW_FFTFRAMEGENERATOR_H
#define RASDR_FFTW_FFTFRAMEGENERATOR_H

#include "StreamCircularBuffer.h"

#include <fftw3.h>
#include <iterator>
#include <algorithm>
#include <chrono>
#include <thread>
#include <complex>
#include <deque>


class FFTFrameGenerator {
private:
    StreamCircularBuffer *rawIQBuffer;
    StreamCircularBuffer *framesBuffer;
    fftw_complex *fftw_in, *fftw_out;
    fftw_plan p;
    int blocks_per_frame, samples;
    StreamCircularBuffer::StreamingParameters* streamParam;
    double center_freq_hz;
    double bin_size_hz;
    double min_freq_hz;
    double max_freq_hz;
    double sample_rate_sps;
    std::string channel;
    StreamCircularBuffer *buffer;
    bool has_audio_stream;


public:
    FFTFrameGenerator();// constructor declaration
    void setBuffers(StreamCircularBuffer *rawIQBuffer, StreamCircularBuffer* framesBuffer);
    void init(StreamCircularBuffer::StreamingParameters* streamingParameters);
    void start(int loops);
    void setAudioStream(StreamCircularBuffer *audiobuffer);
    void process();
    void stop();
};

#endif //RASDR_FFTW_FFTFRAMEGENERATOR_H
