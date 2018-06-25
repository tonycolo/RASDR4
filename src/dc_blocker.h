/*Created by astro on 6/16/18, derived from:
https://github.com/gnuradio/gnuradio/blob/master/gr-filter/lib/dc_blocker_cc_impl.h

This program is free software: you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef RASDR_FFTW_DC_BLOCKER_H
#define RASDR_FFTW_DC_BLOCKER_H

#endif //RASDR_FFTW_DC_BLOCKER_H

#include <deque>
#include <complex>
#include <vector>

class moving_averager_c {
public:
    moving_averager_c(int D);
    ~moving_averager_c();

    std::complex<double> filter(std::complex<double> x);
    std::complex<double> delayed_sig() { return d_out; }

private:
    int d_length;
    std::complex<double> d_out, d_out_d1, d_out_d2;
    std::deque<std::complex<double>> d_delay_line;
};

class dc_blocker
{
    private:
        int d_length;
        bool d_long_form;
        moving_averager_c *d_ma_0;
        moving_averager_c *d_ma_1;
        moving_averager_c *d_ma_2;
        moving_averager_c *d_ma_3;
        std::deque<std::complex<double>> d_delay_line;

    public:
        dc_blocker(int D, bool long_form);

        ~dc_blocker();

        int group_delay();

        int work(int noutput_items,
                 std::vector<std::complex<double>> input_items,
                 std::vector<std::complex<double>> output_items, std::complex<double> out[]);
        //const std::complex<double>
};
