#ifndef PARAMS_HPP
#define PARAMS_HPP

struct Params
{
    float delay_time;
    // The last time value read from the time knob before switching to tap tempo
    float time_knob;
    float feedback;
    float mix;
    float age;
    float stability;
};

#endif  // PARAMS_HPP
