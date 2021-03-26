#include "synth/biquad.hh"

#include <gtest/gtest.h>

/// For validating coefficients: https://www.earlevel.com/main/2013/10/13/biquad-calculator-v2/
/// Note that they use a different coefficient mapping from us. Specifically the b's and a's are flipped

namespace synth {
TEST(BiQuad, LPF) {
    {
        auto res = BiQuadFilter::low_pass_filter(600.0, 6.0, 1.0);
        EXPECT_NEAR(res.b0, 0.0017294824301367212, 1E-8);
        EXPECT_NEAR(res.b1, 0.0034589648602734425, 1E-8);
        EXPECT_NEAR(res.b2, 0.0017294824301367212, 1E-8);
        EXPECT_NEAR(res.a1, -1.878965973994988, 1E-8);
        EXPECT_NEAR(res.a2, 0.885883903715535, 1E-8);
    }
}

//
// #############################################################################
//

TEST(BiQuad, HPF) {
    {
        auto res = BiQuadFilter::high_pass_filter(2000.0, 3.0, 0.2);
        EXPECT_NEAR(res.b0, 0.6607711888045134, 1E-8);
        EXPECT_NEAR(res.b1, -1.3215423776090267, 1E-8);
        EXPECT_NEAR(res.b2, 0.6607711888045134, 1E-8);
        EXPECT_NEAR(res.a1, -1.2942231921461547, 1E-8);
        EXPECT_NEAR(res.a2, 0.34886156307189853, 1E-8);
    }
}

//
// #############################################################################
//

TEST(BiQuad, HPF_process) {
    {
        BiQuadFilter filter;
        filter.set_coeff(BiQuadFilter::high_pass_filter(2000.0, 3.0, 1.0));
        EXPECT_NEAR(filter.process(1.0), 0.8169898522318204, 1E-6);
        EXPECT_NEAR(filter.process(0.9898209799899635), 0.48204258525293475, 1E-6);
        EXPECT_NEAR(filter.process(0.9594911448565836), 0.2093511495340652, 1E-6);
        EXPECT_NEAR(filter.process(0.9096279505973077), -0.002842106651871179, 1E-6);
    }
}
}  // namespace synth
