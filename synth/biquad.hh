#pragma once

#include "synth/samples.hh"

///
/// @brief An implementation of a BiQuad filter to operate on samples
/// For derivation of coefficients: https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html
///

namespace synth {
class BiQuadFilter {
public:
    struct Coeff {
        double b0;  // xn
        double b1;  // xn_1
        double b2;  // xn_2
        double a1;  // yn_1
        double a2;  // yn_2

        void normalize();
    };

public:
    static Coeff low_pass_filter(float f0, float gain, float slope);
    static Coeff high_pass_filter(float f0, float gain, float slope);

public:
    void set_coeff(const Coeff& coeff);
    void process(Samples& samples);

private:
    Coeff coeff_;
    double xn_1_ = 0.f;
    double xn_2_ = 0.f;
    double yn_1_ = 0.f;
    double yn_2_ = 0.f;
};
}  // namespace synth
