//
// Created by astro on 1/28/18.
//



#include "FFTFrameGenerator.h"
#include "dc_blocker.h"

#include <iostream>
#include <complex>


FFTFrameGenerator::FFTFrameGenerator()
{

}

void FFTFrameGenerator::setBuffers(StreamCircularBuffer *raw, StreamCircularBuffer *frames) {
    rawIQBuffer = raw;
    framesBuffer = frames;
    has_audio_stream = false; //default
}

/**
 * Initialize FFT parameters.
 * Compute min frequency and bin size and store in the stream object for use by other classes.
 * @param stream
 */
void FFTFrameGenerator::init(StreamCircularBuffer::StreamingParameters* stream) {
    samples = stream->sample_size_per_block;
    blocks_per_frame = stream->blocks_per_frame;
    printf("blocks_per_frame %d \n", blocks_per_frame);
    center_freq_hz = stream->center_frequency_hz;
    sample_rate_sps = stream->sampling_rate_sps;
    bin_size_hz = sample_rate_sps / samples;
    min_freq_hz = center_freq_hz - (.5 * sample_rate_sps);
    max_freq_hz = center_freq_hz + (.5 * sample_rate_sps);
    stream->bin_hz = bin_size_hz;
    stream->min_freq_hz = min_freq_hz;
    stream->max_freq_hz = max_freq_hz;
    printf("min freq hz %.1f max freq hz %.1f\n",min_freq_hz,max_freq_hz);
    printf("samples per block %d ", samples);
    printf("fft bin size (hz) %.1f\n", bin_size_hz);
    fftw_in = (fftw_complex *) fftw_malloc(sizeof(fftw_complex) * samples );
    fftw_out = (fftw_complex *) fftw_malloc(sizeof(fftw_complex) * samples);
    printf("samples %i\n",samples);
    p = fftw_plan_dft_1d(samples, fftw_in, fftw_out, FFTW_FORWARD, FFTW_MEASURE);
    channel = stream->channel;
}

void FFTFrameGenerator::setAudioStream(StreamCircularBuffer *audiobuffer) {
    buffer = audiobuffer;
    has_audio_stream = true;
}

void FFTFrameGenerator::start(int loops) {
    StreamCircularBuffer::Block frame;
    frame.index = -1;
    int previous_frame = -1;
    bool loop = true;
    float amplitudes[samples];
    double powers[samples];
    double amplitudes2[samples];
    std::complex<double> dcout[samples];
    double max_power;
    int blocks = 0;
    uint64_t first_time = 0;

    dc_blocker *blocker = new dc_blocker(64,true);
    std::vector<std::complex<double>>  dc_in(samples);
    std::complex<double> fill(0,0);

    std::vector<std::complex<double>> dc_out(samples);
    //for (int i = 0; i < samples; i++) {
    //    dc_out.push_back(std::complex<double> (0,0));
    //}
    //= new std::vector<void *>(samples);//, &fill);
    //std::cout << dc_out.size() << " dc_out size\n";
    //std::cout << "blah blah\n";
    while (loop) {
        rawIQBuffer->nextBlock(&frame);
        if (frame.index == previous_frame || frame.index < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        previous_frame = frame.index;
        if (blocks==0) {
            first_time = frame.timestamp;
        }
        int zero_count = 0;
        for (int i = 0; i < samples; i++) {
            double ii = (double) frame.data[i*2];
            double jj = (double) frame.data[i*2 + 1];
            if (has_audio_stream) {
                amplitudes2[i] = ii*ii+jj*jj;//amplitude;
            }
            /* dc bias removal code */
            std::complex<double> c(ii,jj);
            dc_in[i] = c;
            /* end dc bias removal code */
            fftw_in[i][0] = ii;
            fftw_in[i][1] = jj;
        }
        if (has_audio_stream) {
            buffer->pushBlockFloat(amplitudes2, samples,first_time);
        }
        /*blocker->work(samples,dc_in, dc_out, dcout);
        for (int i = 0; i < samples; i++) {
            std::complex<double> c = dcout[i];
            fftw_in[i][0] = c.real(); //normalize 12-bit for -1 to 1 values
            fftw_in[i][1] = c.imag();
        }*/
        //dc_in.clear();
        fftw_execute(p);
        int dc_index = -999;
        float power_max = -99999;
        for (int i = 0; i < samples; i++) {
            double iqsq = fftw_out[i][0] * fftw_out[i][0] + fftw_out[i][1] * fftw_out[i][1];
            double amplitude = sqrt(iqsq)/samples;
            amplitudes[i] = amplitudes[i] + amplitude;
        }
        blocks = blocks + 1;
        if (blocks == blocks_per_frame) {
            double prev_x = 0;
            double prev_y = 0;
            double prev_power = 0;
            for (int i = 0; i < samples; i++) {
                //compute average amplitude, then average power
                double avg_power = 20 * log10(amplitudes[i]/blocks);
                amplitudes[i] = 0;
                powers[i] = avg_power;
            }
            framesBuffer->pushBlockFloat(powers, samples,first_time);
            blocks = 0;
        }
    }
}



