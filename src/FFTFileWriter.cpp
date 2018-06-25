//
// Created by astro on 1/30/18.
//

#include "FFTFileWriter.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <sstream>
#include <math.h>

using namespace std;

    std::string fname;
    StreamCircularBuffer* frames;
    StreamCircularBuffer* frames2;
    int frames_limit;
    bool write;
    StreamCircularBuffer::StreamingParameters *stream;
    StreamCircularBuffer::StreamingParameters *stream2;
    GNUPlotPipe * gp1; // channel 1
    GNUPlotPipe * gp2; // channel 2
    std::chrono::microseconds start_micros; //sample 0 start streaming time
    std::chrono::seconds start_seconds;// = std::chrono::duration_cast<std::chrono::seconds>(milliseconds);

    FFTFileWriter::FFTFileWriter() {

    }

    void FFTFileWriter::init(std::string outfileprefix,StreamCircularBuffer* framesBuffer1, StreamCircularBuffer* framesBuffer2,
                             StreamCircularBuffer::StreamingParameters *parameters, StreamCircularBuffer::StreamingParameters *parameters2,
                             int limit) {
        frames = framesBuffer1;
        frames2 = framesBuffer2;
        fname = outfileprefix;
        frames_limit = limit;
        write = true;
        stream = parameters;
        stream2 = parameters2;
        /* create a timestamp for unique file name component */
        std::time_t dt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::ctime(&dt);
        fname.append("-");
        //fname.append(std::ctime(&dt));
        fname.append(std::to_string(dt));
        fname.append(".txt");
        initGNUGraphs();
    }


    /**
     * Construct GNU Plot graph frames for FFT frames display.
     * One day the yrange [-125:-65] should not be hardcoded
     */
    void FFTFileWriter::initGNUGraphs() {
        gp1 = new GNUPlotPipe();
        gp2 = new GNUPlotPipe();
        gp1->write("set style line 1 lc rgb '#dd181f' lt 1 lw .5 pt 5 ps .4\n");
        gp1->write("set yrange [-125:-65]\n");
        gp2->write("set style line 1 lc rgb '#0000ff' lt 1 lw .5 pt 5 ps .4\n");
        gp2->write("set yrange [-125:-65]\n");
        /* construct axis labels */
        double min_mhz = stream->min_freq_hz/1e6;
        double mhz = stream->min_freq_hz/1e6;
        double max_mhz = stream->max_freq_hz/1e6;
        double my_ceil = ceil(mhz)-.5;
        int i = 0;
        std::string xtics;
        xtics.append("set xtics(");
        while (my_ceil <= ceil(max_mhz)) {
            //compute index from zero point
            int index = ((my_ceil-min_mhz) * 1e6) / stream->bin_hz;
            std::string str_index = std::to_string(index);
            std::string s(16, '\0');
            auto written = std::snprintf(&s[0], s.size(), "%.1f", my_ceil);
            s.resize(written);
            xtics.append("\"");
            xtics.append(s);
            xtics.append("\" ");
            xtics.append(str_index);
            xtics.append(",");
            //std::cout << xtics << "\n";
            my_ceil = my_ceil + .5;
        }
        xtics.append(")\n");
        gp1->write(xtics.c_str());
        gp2->write(xtics.c_str());
        //{/Times:Bold boldface-newfont}
        std::string title1 = "set xlabel \"{/Consolas:Bold RX ";
        title1.append(stream->channel);
        title1.append(" ");
        title1.append(stream->band);
        title1.append("}");
        title1.append("\"\n");
        gp1->write(title1.c_str());

        std::string title2 = "set xlabel \"{/Consolas:Bold RX ";
        title2.append(stream2->channel);
        title2.append(" ");
        title2.append(stream2->band);
        title2.append("}");
        title2.append("\"\n");
        gp2->write(title2.c_str());

        gp1->write("set nokey\n");
        gp2->write("set nokey\n");

        gp1->write("set terminal qt position 0,0 nopersist\n");
        gp2->write("set terminal qt position 800,0 nopersist\n");

        //gp1->write("set multiplot layout 2,2 rowsfirst\n");
        //gp1->write("set xtics (\"406\" 0)\n");
        //std::str min = stream->min_freq_hz
    }

    std::string FFTFileWriter::constructFrameLine(StreamCircularBuffer::Block block, StreamCircularBuffer::StreamingParameters *streamx,
                                                  GNUPlotPipe * gp) {
        std::string line;
        //compute elapsed time since start
        double sec = block.timestamp/stream->sampling_rate_sps;
        std::chrono::microseconds microseconds_elapsed((long)round(sec*1000000));
        double sec_elapsed = microseconds_elapsed.count() / 1000000.0;
        line.append(std::to_string(sec_elapsed));  //mseconds_elapsed.count())
        //cout << block.timestamp << " " << stream->effective_sample_rate  << " -> " << sec << " " << mseconds_elapsed.count() << "\n";
        //line.append(std::to_string(block.timestamp));
        line.append(",");
        line.append(streamx->channel);
        line.append(",");
        line.append(std::to_string(streamx->center_frequency_hz));
        line.append(",");
        line.append(std::to_string(streamx->min_freq_hz));
        line.append(",");
        line.append(std::to_string(streamx->bin_hz));
        line.append(",");
        std::stringstream ss;
        //cout << std::to_string(block.timestamp) << "\n";


        int cf = ((stream->sample_size_per_block/2)*stream->bin_hz)/1e6;
        gp->write("plot '-' with linespoints ls 1\n");

        for (int i = stream->sample_size_per_block/2; i < stream->sample_size_per_block; i++) {
            std::string s(16, '\0');
            auto written = std::snprintf(&s[0], s.size(), "%.2f", block.data[i]);
            s.resize(written);
            line.append(s);
            line.append(",");
            gp->writef("%.2f\n", block.data[i]);
        }
        for (int i = 0; i < (stream->sample_size_per_block/2)-1; i++) {
            std::string s(16, '\0');
            auto written = std::snprintf(&s[0], s.size(), "%.2f", block.data[i]);
            s.resize(written);
            line.append(s);
            line.append(",");
            gp->writef("%.2f\n", block.data[i]);
        }
        //write very last element
        std::string s(16, '\0');
        auto written = std::snprintf(&s[0], s.size(), "%.2f",block.data[(streamx->sample_size_per_block/2)-1]);
        s.resize(written);
        line.append(s);
        line.append("\n");
        gp->write("e\n");
        gp->flush();
        return line;
    }
    /**
     * Write up to frames_limit frames;
     * To Do:  obtain absolute time stamp of first block.
     */
    void FFTFileWriter::start() {
        ofstream myfile (fname);
        if (!myfile.is_open()) {
            cout << "Unable to open file for fft writing!\n";
            return;
        }
        StreamCircularBuffer::Block block;
        StreamCircularBuffer::Block block2;
        block.index = -1;
        block2.index = -1;
        int previous_index = -1;
        int previous_index_2 = -1;
        bool flag_1 = true;
        bool flag_2 = true;
        bool first_frame = true;
        char mbstr[100];
        while (write) {
            bool flag_1 = true;
            bool flag_2 = true;
            //obtain frames until none are available, then sleep, and try again
            frames->nextBlock(&block);
            frames2->nextBlock(&block2);
            if (block.index == previous_index || block.index < 0 ) {
                flag_1 = false;
            }
            if (block2.index == previous_index_2 || block2.index < 0 ) {
                flag_2 = false;
            }
            if (!flag_1 && !flag_2) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }
            if ((flag_1 || flag_2) && first_frame) {
                start_micros = std::chrono::duration_cast<std::chrono::microseconds>(stream->first_time.time_since_epoch());

                std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds>(start_micros);
                std::time_t t = s.count();
                double fractional_seconds = (double)(start_micros.count() % 1000000)/1000000.0;

                //std::chrono::duration<float> float_seconds = std::chrono::duration_cast<std::chrono::float_seconds>(stream->first_time.time_since_epoch());

                        //double fractional_seconds = (start_micros.count() - floor(start_micros.count() / 1000000.0))/1000000.0;
                start_seconds =  std::chrono::duration_cast<std::chrono::seconds>(start_micros);


                std::strftime(mbstr, sizeof(mbstr), "%FT%TZ", std::gmtime(&t)); //"%A %c %j
                myfile << "stream-start,fractional-seconds-start,decimation-ratio,lowpass_bandwidth_hz,rf-sampling_rate_sps,effective-sampling-rate,stream-samples,fft-frame-size\n";
                myfile << mbstr << "," << fractional_seconds << "," << stream->decimation << "," << stream->lowpass_bandwidth_hz << "," << stream->sampling_rate_sps << "," << stream->effective_sample_rate << "," << stream->sample_size_per_block << "," << stream->blocks_per_frame << "\n";
                myfile << "elapsed-seconds,channel,center-frequency-hz,minimum-frequency-hz,channel-size-hz\n";
                first_frame = false;
                //millisec_first_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(stream->stream_start_time.time_since_epoch());
            }
            if (flag_1) {
                previous_index = block.index;
                myfile << constructFrameLine(block,stream, gp1);
            }
            if (flag_2) {
                previous_index_2 = block2.index;
                myfile << constructFrameLine(block2,stream2, gp2);
            }
        }
        myfile.close();
    };

