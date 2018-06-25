//
// Created by astro on 1/30/18.
//

#ifndef RASDR_FFTW_FFTFILEWRITER_H
#define RASDR_FFTW_FFTFILEWRITER_H

#include <bits/unique_ptr.h>
#include "StreamCircularBuffer.h"
#include "gnuPlotPipe.h"

class FFTFileWriter {

public:

    FFTFileWriter();
    void init(std::string outfileprefix,StreamCircularBuffer* frames1, StreamCircularBuffer* frames2, StreamCircularBuffer::StreamingParameters *parameters, StreamCircularBuffer::StreamingParameters *parameters2, int limit=99999999);
    void initGNUGraphs();
    std::string constructFrameLine(StreamCircularBuffer::Block block, StreamCircularBuffer::StreamingParameters* streamx, GNUPlotPipe * gp);
    void start();
};


#endif //RASDR_FFTW_FFTFILEWRITER_H
