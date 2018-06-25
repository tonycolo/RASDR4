//
// Created by astro on 6/16/18.
// adopted from https://github.com/gnuradio/gnuradio/blob/master/gr-filter/lib/dc_blocker_cc_impl.cc
//

#include "dc_blocker.h"
#include <iostream>

moving_averager_c::moving_averager_c(int D)
        : d_length(D), d_out(std::complex<double>(0,0)), d_out_d1(std::complex<double>(0,0)), d_out_d2(std::complex<double>(0,0))
{
    d_delay_line = std::deque<std::complex<double>>(d_length-1, std::complex<double>(0,0));
}

moving_averager_c::~moving_averager_c()
{
}

std::complex<double> moving_averager_c::filter(std::complex<double> x)
{
    d_out_d1 = d_out;
    d_delay_line.push_back(x);
    d_out = d_delay_line[0];
    d_delay_line.pop_front();

    std::complex<double> y = x - d_out_d1 + d_out_d2;
    d_out_d2 = y;
    return (y / (double)(d_length));
}

dc_blocker::dc_blocker(int D, bool long_form) : d_length(D), d_long_form(long_form)

{
    if(d_long_form) {
        d_ma_0 = new moving_averager_c(D);
        d_ma_1 = new moving_averager_c(D);
        d_ma_2 = new moving_averager_c(D);
        d_ma_3 = new moving_averager_c(D);
        d_delay_line = std::deque<std::complex<double>>(d_length-1, std::complex<double>(0,0));
    }
    else {
        d_ma_0 = new moving_averager_c(D);
        d_ma_1 = new moving_averager_c(D);
        d_ma_2 = NULL;
        d_ma_3 = NULL;
    }
}

dc_blocker::~dc_blocker()
{
    if(d_long_form) {
        delete d_ma_0;
        delete d_ma_1;
        delete d_ma_2;
        delete d_ma_3;
    }
    else {
        delete d_ma_0;
        delete d_ma_1;
    }
}

int dc_blocker::group_delay()
{
    if(d_long_form)
        return (2*d_length-2);
    else
        return d_length - 1;
}

int dc_blocker::work(int noutput_items,
                     std::vector<std::complex<double>> input_items,
                     std::vector<std::complex<double>> output_items, std::complex<double> out[])
{
    if (d_long_form) {
        std::complex<double> y1, y2, y3, y4, d;
        for (int i = 0; i < noutput_items; i++) {
            y1 = d_ma_0->filter(input_items[i]);
            y2 = d_ma_1->filter(y1);
            y3 = d_ma_2->filter(y2);
            y4 = d_ma_3->filter(y3);

            d_delay_line.push_back(d_ma_0->delayed_sig());
            d = d_delay_line[0];
            d_delay_line.pop_front();
            output_items[i] = d-y4;
            out[i] = d-y4;
        }
    }
    else {
        std::complex<double> y1, y2;
        for(int i = 0; i < noutput_items; i++) {
            y1 = d_ma_0->filter(input_items[i]);
            y2 = d_ma_1->filter(y1);
            output_items[i] = d_ma_0->delayed_sig() - y2;
        }
    }
    //std::cout << output_items[1004] << " *\n";
    return noutput_items;
}