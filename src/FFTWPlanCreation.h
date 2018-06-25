//
// Created by astro on 1/31/18.
//

#ifndef RASDR_FFTW_FFTWPLANCREATION_H
#define RASDR_FFTW_FFTWPLANCREATION_H


#include <fftw3.h>

class FFTWPlanCreation {

public:

    fftw_plan createPlan(int samples, fftw_complex * fftw_in, fftw_complex *out);
};


#endif //RASDR_FFTW_FFTWPLANCREATION_H
