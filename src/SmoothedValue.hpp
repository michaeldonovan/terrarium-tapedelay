/**
 * Based on CParamSmooth from the musicdsp.org mailing list
 * https://www.musicdsp.org/en/latest/Filters/257-1-pole-lpf-for-smooth-parameter-changes.html?highlight=smooth
 */

#include <cmath>

#include "DspUtils.hpp"
namespace mbdsp
{

template <class Float_t>
class SmoothedValue
{
public:
    void Init(Float_t sample_rate, Float_t smooth_time_ms = 100, Float_t initial_val = 0)
    {
        a_ = std::exp(-mbdsp::twopi() / (smooth_time_ms * 0.001 * sample_rate));
        b_ = 1.0 - a_;
        curr_ = initial_val;
        SetTarget(initial_val);
    }

    inline Float_t Process()
    {
        if(done_smoothing_) { return target_; }

        if(curr_ == target_)
        {
            done_smoothing_ = true;
            return target_;
        }

        curr_ = c_ + (curr_ * a_);
        return curr_;
    }

    inline void SetTarget(float target)
    {
        target_ = target;
        c_ = target_ * b_;
        done_smoothing_ = false;
    }

    inline float GetCurrent() { return curr_; }

private:
    Float_t a_;
    Float_t b_;
    Float_t c_;
    Float_t target_;
    Float_t curr_;
    bool done_smoothing_;
};

}  // namespace mbdsp