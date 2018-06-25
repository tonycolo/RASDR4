//
// Created by astro on 2/11/18.
//


#ifndef RASDR_FFTW_AUDIODEMODULATE_H
#define RASDR_FFTW_AUDIODEMODULATE_H

#include <RtAudio.h>
#include "AudioDemodulate.h"
#include "StreamCircularBuffer.h"


class AudioDemodulate  {


public:
    AudioDemodulate();
    void init(StreamCircularBuffer *buffer, int samples);
    int saw( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
             double streamTime, RtAudioStreamStatus status, void *userData );

};

int amplitudes( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
                double streamTime, RtAudioStreamStatus status, void *userData );

#endif //RASDR_FFTW_AUDIODEMODULATE_H
