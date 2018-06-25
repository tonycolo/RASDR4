//
// Created by astro on 3/23/18.
//

#ifndef RASDR_FFTW_SETTINGS_H
#define RASDR_FFTW_SETTINGS_H

#include "Poco/Util/PropertyFileConfiguration.h"

#include <vector>

class Properties {


public:

    struct rasdr4 {
        std::vector<double> center_frequencies;

        std::string band; //L H W
        double center_frequency_hz;
        double sampling_rate_sps;
        int decimation;
        int sample_size_per_block;
        int blocks_per_frame;
        double bin_hz;
        double min_freq_hz;
        double lowpass_bandwidth_hz;
        double cgen_freq_hz;  //adc clock rate
        int lna_gain;

    };
    Properties(const std::string & path);
    Poco::Util::PropertyFileConfiguration* getPropertyFileConfiguration();


private:
    Poco::Util::PropertyFileConfiguration* properties;

};


#endif //RASDR_FFTW_SETTINGS_H
