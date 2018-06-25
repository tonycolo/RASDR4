//
// Created by astro on 1/28/18.
//

#ifndef RASDR_FFTW_CIRCULARBUFFER_H
#define RASDR_FFTW_CIRCULARBUFFER_H


#include <array>
#include <iostream>
#include <chrono>
#include <Poco/UUIDGenerator.h>

/**
 * A class for a single producer to share preallocated buffer arrays
 * and for multiple consumers to read.
 * The primary data structure is a circular buffer.
 *
 */
class StreamCircularBuffer {

    float  s[2048][16384*2];  //max number of blocks of I+Q samples
    uint64_t * timestamps;
    int sampleCount;          //array length for each element
    int sample_dim;           //sample dimensions, 2 for I+Q, 1 for frames
    int size;
    int head;                 //current index for latest data
    int tail;                 //oldest index by a consuming module
    int registry[16];
    int registry_seq;
    std::string uuid;

public:

    struct Block {
        int id;
        int index;
        float *data;
        uint64_t timestamp;
    };

    struct StreamingParameters {
        std::string channel;
        std::string band; //L H W
        double center_frequency_hz;
        double lowpass_bandwidth_hz;
        double sampling_rate_sps;
        int sample_size_per_block;
        int blocks_per_frame;
        double bin_hz;
        double min_freq_hz;
        double max_freq_hz;
        int decimation; //decimation ratio
        int lna_gain;
        /* derived/measured */
        std::chrono::high_resolution_clock::time_point first_time;
        double effective_sample_rate; //or rf sample rate hz
    };

    StreamCircularBuffer(std::size_t sampleCount,std::size_t size1, int sample_dimensions) : sampleCount(sampleCount), size(size1) {// constructor declaration
        sample_dim = sample_dimensions;
        head = -1;
        size = size1;
        //s = new float[size*sampleCount*sample_dim];
        timestamps = new uint64_t[size];
        tail = 9999;
        registry_seq = 0;
        uuid = Poco::UUIDGenerator().createRandom().toString();
    }

    void pushBlockInt(int16_t *array, int samples, uint64_t timestamp) {
        if ((head+1) == tail) { //throw away samples
            printf("** samples thrown away %i ***\n",tail);
            return;
        }
        if (head+1 ==size ) {
            for (int i = 0; i < samples; i++) {
                s[0][sample_dim*i] = (float)array[sample_dim*i];
                s[0][sample_dim*i+1] = (float)array[sample_dim*i+1];;
            }
            timestamps[0] = timestamp;
            head = 0;
        } else {
            int temp = head + 1;
            for (int i = 0; i < samples; i++) {
                s[temp][sample_dim*i] = (float)array[sample_dim*i];
                s[temp][sample_dim*i+1] = (float)array[sample_dim*i+1];
                //if ()
                //printf("%.4f,%.4f\n",s[head][sample_dim*i],s[head][sample_dim*i+1]);
            }
            //std::copy(array,array+(samples*sample_dim),s[head]);
            timestamps[temp] = timestamp;
            head = head + 1;
        }
    }

    /**
     * Copy the frame/samples, accounting for I+Q length.
     * Update the index.
     */
    void pushBlock(float *array, int samples, uint64_t timestamp) {
        if ((head+1) == tail) { //throw away samples
            printf("** samples thrown away %i ***\n",tail);
            return;
        }
        if (head+1 ==size ) {
            for (int i = 0; i < samples; i++) {
                s[0][sample_dim*i] = (float)array[sample_dim*i];
                s[0][sample_dim*i+1] = (float)array[sample_dim*i+1];;
            }
            timestamps[0] = timestamp;
            head = 0;
        } else {
            int temp = head + 1;
            for (int i = 0; i < samples; i++) {
                s[temp][sample_dim*i] = (float)array[sample_dim*i];
                s[temp][sample_dim*i+1] = (float)array[sample_dim*i+1];
            }
            timestamps[temp] = timestamp;
            head = head + 1;
        }
    }

    void pushBlockFloat(double *array, int samples, uint64_t timestamp) {
        if ((head+1) == tail) { //throw away samples
            printf("** samples thrown away %i ***\n",tail);
            return;
        }
        if (head+1 ==size ) {
            std::copy(array,array+(samples*sample_dim),s[0]);
            timestamps[0] = timestamp;
            head = 0;
        } else {
            std::copy(array,array+(samples*sample_dim),s[head+1]);
            timestamps[head+1] = timestamp;
            head = head + 1;
        }
    }

    /*
     * Obtain the next frame given the most recent frame already obtained.
     * The supplied frame must have an index of the most recent frame obtained.
     * If a frame has never been obtained, its index must be < 0.
     * The index in the frame will be incremented if there is another
     * unaccessed frame available.
     */
    void nextBlock(StreamCircularBuffer::Block *block) {
        if (head == -1) { //no data available
            return;
        }
        if (block->index == head) {
            return; //have most recent data
        }
        block->index++;

        if (block->index==size) {
            block->index = 0;
        }
        block->data = s[block->index]; //advance to next, using ampersand for array address
        block->timestamp = timestamps[block->index];
        if (block->index < tail ) {
            tail = block->index;
        }
    }

    int currentIndex() {
        return head;
    }

    int registerConsumer() {
        int id = registry_seq;
        registry[id] = -1;
        return id;
    }

    std::string getUUID() {
        return uuid;
    }

};


#endif //RASDR_FFTW_CIRCULARBUFFER_H
