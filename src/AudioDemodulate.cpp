//
// Created by astro on 2/11/18.
//
#include <iostream>
#include "RtAudio.h"
#include "AudioDemodulate.h"
#include <thread>


bool play;
RtAudio *audio = 0;
StreamCircularBuffer::Block *block;
int previous_index;

typedef struct {
    unsigned int	nRate;		/* Sampling Rate (sample/sec) */
    unsigned int	nChannel;	/* Channel Number */
    unsigned int	nFrame;		/* Frame Number of Wave Table */
    float		    *wftable;	/* Wave Form Table(interleaved) */
    unsigned int	cur;		/* current index of WaveFormTable(in Frame) */
} CallbackData;

AudioDemodulate::AudioDemodulate()

{
}

int AudioDemodulate::saw( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
         double streamTime, RtAudioStreamStatus status, void *userData )
{
    unsigned int i, j;
    double *buffer = (double *) outputBuffer;
    double *lastValues = (double *) userData;
    if ( status )
        std::cout << "Stream underflow detected!" << std::endl;
    // Write interleaved audio data.
    for ( j=0; j<nBufferFrames; j++ ) {
        //for ( j=0; j<2; j++ ) {
            *buffer++ = lastValues[j];
            lastValues[j] += 0.005 * (j+1+(j*0.5));
            if ( lastValues[j] >= 1.0 ) lastValues[j] -= 2.0;
        //}
    }
    return 0;
}

/**
 * Callback routine to read streaming I+Q.
 * @param outputBuffer data structure for audio data
 * @param inputBuffer  not used
 * @param nBufferFrames how many samples for audio
 * @param streamTime not used
 * @param status  not used
 * @param data
 * @return
 */
 int amplitudes (void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
                                        double streamTime, RtAudioStreamStatus status, void *data ) {
     double *buffer2 = (double *) outputBuffer;
     StreamCircularBuffer * buffer = (StreamCircularBuffer *)data;
     //double d = 0;
     // unsigned char buf[sizeof d] = {0};
     int i = 0;
     buffer->nextBlock(block);
     while  (block->index == previous_index || block->index < 0 ) {
         std::this_thread::sleep_for (std::chrono::milliseconds(2));
         buffer->nextBlock(block);
     }
     previous_index = block->index;
     while (i < nBufferFrames) {
         //*buffer2++ = block->data[i];
         //float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
         *buffer2++ = block->data[i] *100000;
         i = i+ 1;
     }
     return 0;
}

/**
 * Initialize audio device, register call back to for AM, and start.
 * @param audiobuffer source of streaming I+Q
 * @param samples number of samples per block
 */
void AudioDemodulate::init(StreamCircularBuffer *audiobuffer, int samples) {
    play = true;
    /* set up streaming source */
    block = new StreamCircularBuffer::Block();
    block->index = -1;
    previous_index = -1;

    RtAudio dac;
    // Determine the number of devices available
    unsigned int devices = dac.getDeviceCount();
    // Scan through devices for various capabilities
    RtAudio::DeviceInfo info;
    for ( unsigned int i=0; i<devices; i++ ) {
        info = dac.getDeviceInfo( i );
        if ( info.probed == true ) {
            /*data we could print save etc.
            std::cout << "device = " << i;
            std::cout << ": maximum output channels = " << info.outputChannels << "\n";
            std::cout << ": native formats          = " << info.nativeFormats << "\n";
            std::cout << ": preferred sample rate   = " << info.preferredSampleRate << "\n";
            std::cout << ": sample rates            = " << info.sampleRates.size() << "\n";
            */
        }
    }

    if ( dac.getDeviceCount() < 1 ) {
        std::cout << "\nNo audio devices found!\n";
        exit( 0 );
    }
    RtAudio::StreamParameters parameters;
    parameters.deviceId = dac.getDefaultOutputDevice();
    parameters.nChannels = 1;
    parameters.firstChannel   = 0;
    unsigned int sampleRate   = 44100;
    unsigned int bufferFrames = samples; // audio buffer size is the same as streaming sample size
    int nBuffers= 1;                     //number of internal buffers used by device
    int device = 0;                      // 0 indicates the default or first available device

    try {
        audio = new RtAudio();
        //audio->openStream( &parameters, NULL, RTAUDIO_FLOAT64, sampleRate, &bufferFrames,&AudioDemodulate::saw,&data); //AudioDemodulate::amplitudes
        audio->openStream( &parameters, NULL, RTAUDIO_FLOAT64, sampleRate,&bufferFrames, &amplitudes, (void *)audiobuffer);         //AudioDemodulate::amplitudes
        audio->startStream();
    } catch ( RtAudioError& e ) {
        e.printMessage();
        exit( 0 );
    }
    return;
}
