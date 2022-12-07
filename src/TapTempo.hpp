#ifndef TAPTEMPO_HPP
#define TAPTEMPO_HPP

#include <array>
#include <type_traits>

namespace mbdsp
{
template <class NowFn_t>
class TapTempo
{
public:
    using TimeVal_t = decltype(std::declval<NowFn_t>()());
    static_assert(std::is_arithmetic<TimeVal_t>::value, "NowFn_t must return an arithmetic type");

    /**
     * @param now_fn: A function that will return the current time
     * @param max_duration: The maximum time between taps that will be
     * considered
     */
    void Init(NowFn_t now_fn, TimeVal_t max_duration)
    {
        now_fn_ = now_fn;
        max_dur_ = max_duration;
        Reset();
    }

    TimeVal_t Tap()
    {
        auto now = now_fn_();
        if(last_ == 0)
        {
            last_ = now;
            return 0;
        }

        auto duration = now - last_;
        last_ = now;

        // if this is the first tap in a while, we'll start our calculations
        // over
        if(duration > max_dur_)
        {
            durations_ = {};
            write_ptr_ = 0;
            return 0;
        }
        else
        {
            write(duration);
            return GetBeatLength();
        }
    }

    TimeVal_t GetBeatLength() const
    {
        // calculate mean of taps
        TimeVal_t accum = 0;
        TimeVal_t num_durs = 0;
        for(const auto& dur : durations_)
        {
            if(dur <= 0) { continue; }

            accum += dur;
            num_durs++;
        }
        return accum / num_durs;
    }

    void Reset()
    {
        durations_ = {};
        write_ptr_ = 0;
        last_ = 0;
    }

private:
    inline void write(TimeVal_t duration)
    {
        durations_[write_ptr_] = duration;
        write_ptr_ = (write_ptr_ + 1) % durations_.size();
    }

    NowFn_t now_fn_;
    TimeVal_t last_ = 0;
    std::array<TimeVal_t, 4> durations_;
    size_t write_ptr_ = 0;
    TimeVal_t max_dur_;
};
}  // namespace mbdsp

#endif  // TAPTEMPO_HPP
