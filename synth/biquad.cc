#include "synth/biquad.hh"

#include <cmath>
#include <iostream>
#include <sstream>

namespace synth {
namespace {
double compute_A(float gain) { return std::pow(10.0, gain / 40.0); }
double compute_w(float f0) { return 2 * M_PI * f0 / Samples::kSampleRate; }
double compute_alpha(double w, float gain, float slope) {
    const double A = compute_A(gain);
    return 0.5 * std::sin(w) * std::sqrt((A + 1 / A) * (1 / slope - slope) + 2);
}
}  // namespace

//
// #############################################################################
//

BiQuadFilter::Coeff BiQuadFilter::low_pass_filter(float f0, float gain, float slope) {
    const double w = compute_w(f0);
    const double alpha = compute_alpha(w, gain, slope);

    const double cos = std::cos(w);

    Coeff coeff;
    coeff.b1 = 1 - cos;
    coeff.b0 = 0.5 * coeff.b1;
    coeff.b2 = coeff.b0;
    coeff.a1 = -2 * cos;
    coeff.a2 = 1 - alpha;

    double inv_a0 = 1 / (1 + alpha);
    coeff.b0 *= inv_a0;
    coeff.b1 *= inv_a0;
    coeff.b2 *= inv_a0;
    coeff.a1 *= inv_a0;
    coeff.a2 *= inv_a0;
    return coeff;
}

//
// #############################################################################
//

BiQuadFilter::Coeff BiQuadFilter::high_pass_filter(float f0, float gain, float slope) {
    slope = std::max(1E-3f, slope);
    const double w = compute_w(f0);
    const double alpha = compute_alpha(w, gain, slope);

    const double cos = std::cos(w);

    Coeff coeff;
    coeff.b1 = -(1 + cos);
    coeff.b0 = -0.5 * coeff.b1;
    coeff.b2 = coeff.b0;
    coeff.a1 = -2 * cos;
    coeff.a2 = 1 - alpha;

    double inv_a0 = 1 / (1 + alpha);
    coeff.b0 *= inv_a0;
    coeff.b1 *= inv_a0;
    coeff.b2 *= inv_a0;
    coeff.a1 *= inv_a0;
    coeff.a2 *= inv_a0;

    return coeff;
}

//
// #############################################################################
//

BiQuadFilter::Coeff BiQuadFilter::coeff(const Type type, float f0, float gain, float slope) {
    switch (type) {
        case Type::kHpf:
            return high_pass_filter(f0, gain, slope);
        case Type::kLpf:
            return high_pass_filter(f0, gain, slope);
        default:
            throw std::runtime_error("Unknown coefficient type in BiQuadFilter::coeff()");
    }
}

//
// #############################################################################
//

void BiQuadFilter::set_coeff(const Coeff& coeff) { coeff_ = coeff; }
void BiQuadFilter::set_coeff(const Type type, float f0, float gain, float slope) {
    set_coeff(coeff(type, f0, gain, slope));
}

//
// #############################################################################
//

float BiQuadFilter::process(float xn) {
    const auto& [b0, b1, b2, a1, a2] = coeff_;

    double yn = b0 * xn + b1 * xn_1_ + b2 * xn_2_ - a1 * yn_1_ - a2 * yn_2_;

    if (std::isnan(yn)) {
        std::stringstream ss;
        ss << "BiQuadFilter::process() found nan! ";
        ss << "xn: " << xn << ", xn_1: " << xn_1_ << ", xn_2: " << xn_2_ << ", yn: " << yn << ", yn_1: " << yn_1_
           << ", yn_2: " << yn_2_;
        ss << "b0: " << b0 << ", b1: " << b1 << ", b2: " << b2 << ", a1: " << a1 << ", a2: " << a2;
        throw std::runtime_error(ss.str());
    }

    xn_2_ = xn_1_;
    yn_2_ = yn_1_;
    xn_1_ = xn;
    yn_1_ = yn;
    return yn;
}
}  // namespace synth
