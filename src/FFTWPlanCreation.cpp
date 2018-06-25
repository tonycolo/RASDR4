//
// Created by astro on 1/31/18.
//

#include "FFTWPlanCreation.h"

fftw_plan FFTWPlanCreation::createPlan(int samples, fftw_complex *fftw_in, fftw_complex *out) {
    return fftw_plan_dft_1d(samples, fftw_in, out, FFTW_FORWARD, FFTW_ESTIMATE);
}
