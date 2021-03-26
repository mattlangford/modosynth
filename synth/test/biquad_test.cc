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
}  // namespace synth
