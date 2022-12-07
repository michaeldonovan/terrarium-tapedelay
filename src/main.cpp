#include <random>

#include "Delay.hpp"
#include "DspUtils.hpp"
#include "TapTempo.hpp"
#include "Tape.hpp"
#include "TapeAttrs.hpp"
#include "daisy_petal.h"
#include "daisysp.h"
#include "terrarium.h"
// #include "Vibrato.hpp"

using namespace daisy;
using daisysp::Oscillator;
using terrarium::Terrarium;

constexpr auto DAISY_SR = SaiHandle::Config::SampleRate::SAI_48KHZ;

constexpr float SampleRate()
{
    switch(DAISY_SR)
    {
        case SaiHandle::Config::SampleRate::SAI_96KHZ:
            return 96000;
            break;
        case SaiHandle::Config::SampleRate::SAI_48KHZ:
            return 48000;
            break;
        case SaiHandle::Config::SampleRate::SAI_32KHZ:
            return 32000;
            break;
        case SaiHandle::Config::SampleRate::SAI_16KHZ:
            return 16000;
            break;
        case SaiHandle::Config::SampleRate::SAI_8KHZ:
            return 8000;
            break;
    }
};

constexpr auto MIN_DELAY_SEC = 0.1f;
constexpr auto MAX_DELAY_SEC = 1.f;
constexpr size_t MIN_DELAY = SampleRate() * MIN_DELAY_SEC;
constexpr size_t MAX_DELAY = SampleRate() * MAX_DELAY_SEC;

constexpr auto DELAY_SMOOTH_MS = 500.f;

constexpr auto WOW_FREQ = 0.4f;
constexpr auto FLUTTER_FREQ = 25.f;
constexpr float WOW_MAX_AMP = .0092 * SampleRate();
constexpr float FLUTTER_MAX_AMP = 0.00085 * SampleRate();

DaisyPetal hw;

bool effectOn = true;
bool tails = true;

Led bypass_led;
Led tempo_led;

Parameter mix, time, feedback, lpParam, hpParam, driveParam, satParam, stabParam, ageParam;
float mixVal;

// processors
Delay<MAX_DELAY> delay;
float currDelay;
float feedbackVal;

// The currently active delay time
float delay_time;

// The last time value read from the time knob before switching to tap tempo
float last_time_knob_val;

mbdsp::TapTempo<decltype(&daisy::System::GetNow)> tap_tempo;
bool using_tap = false;

Tape tape;

TapeAttrs tapeAttrs;

Oscillator wow_osc;
Oscillator flutter_osc;
Oscillator tempo_osc;
float tempo_osc_val;

Switch *bypassSw, *tapSw, *tailsSw;

void ProcessSwitches()
{
    hw.ProcessDigitalControls();

    if(bypassSw->RisingEdge())
    {
        effectOn = !effectOn;
        if(effectOn && !tails) { delay.Reset(); }
    }

    if(tapSw->RisingEdge())
    {
        const auto beat_len_ms = tap_tempo.Tap();
        const auto beat_len_samples = beat_len_ms * SampleRate() * .001f;
        if(beat_len_samples >= MIN_DELAY && beat_len_samples <= MAX_DELAY)
        {
            last_time_knob_val = delay_time;
            delay_time = beat_len_samples;
            using_tap = true;
        }
    }

    tempo_led.Set(tempo_osc_val > .95);
    bypass_led.Set(effectOn);

    bypass_led.Update();
    tempo_led.Update();
}

void ProcessKnobs()
{
    hw.ProcessAnalogControls();

    mixVal = mix.Process();

    const auto time_knob_val = time.Process();

    // Get delay time. Prefer tap tempo if available
    if(using_tap)
    {
        // if we're in tap mode and the time knob has been touched, deactivate
        // tap mode
        if(!mbdsp::within_tolerance(last_time_knob_val, time_knob_val, 5.f))
        {
            using_tap = false;
            delay_time = time_knob_val;
            tap_tempo.Reset();
        }
    }
    else { delay_time = time_knob_val; }

    feedbackVal = feedback.Process();

    delay.SetFeedback(feedbackVal);
    delay.SetTime(delay_time);
    delay.EnableInput(effectOn);

    tempo_osc.SetFreq(SampleRate() / delay_time);

    auto stabVal = stabParam.Process();
    wow_osc.SetAmp(WOW_MAX_AMP * stabVal);
    flutter_osc.SetAmp(FLUTTER_MAX_AMP * stabVal);

    // adjust wow and flutter times based on tape speed (delay time)
    auto timing_mult = mbdsp::remap(delay_time, static_cast<float>(MIN_DELAY),
                                    static_cast<float>(MAX_DELAY), -1.f, 1.f);
    auto wow_mod = WOW_FREQ * .2f * timing_mult;
    auto flut_mod = FLUTTER_FREQ * .2f * timing_mult;
    wow_osc.SetFreq(WOW_FREQ + wow_mod);
    flutter_osc.SetFreq(FLUTTER_FREQ + flut_mod);

    auto age = ageParam.Process();
    auto lpFc = mbdsp::remap_exp(1 - age, 1000.f, 6000.f);
    tape.SetLossFilter(lpFc);
    auto tapeDriveDb = mbdsp::remap(age, 6.f, 20.f);
    tape.SetDrive(tapeDriveDb);
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    ProcessKnobs();
    for(size_t i = 0; i < size; i++)
    {
        tempo_osc_val = tempo_osc.Process();
        if(effectOn || tails)
        {
            auto dry = in[0][i];
            auto wet = dry;
            // wet = tape.Process(wet);
            wet = delay.Process(wet);

            auto wowVal = wow_osc.Process();
            auto flutterVal = flutter_osc.Process();
            delay.SetTime(delay_time + wowVal + flutterVal);

            // out[0][i] = wet;

            out[0][i] = dry + mixVal * wet;
        }
        else
        {
            out[0][i] = in[0][i];
            out[1][i] = in[1][i];
        }
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(1024);
    hw.SetAudioSampleRate(DAISY_SR);
    const auto sr = hw.AudioSampleRate();

    bypassSw = &hw.switches[Terrarium::FOOTSWITCH_1];
    tapSw = &hw.switches[Terrarium::FOOTSWITCH_2];
    tailsSw = &hw.switches[Terrarium::SWITCH_1];

    // init leds
    bypass_led.Init(hw.seed.GetPin(Terrarium::LED_1), false);
    bypass_led.Update();
    tempo_led.Init(hw.seed.GetPin(Terrarium::LED_2), false);
    tempo_led.Update();
    tap_tempo.Init(&daisy::System::GetNow, MAX_DELAY_SEC * 1000);

    tape.Init(sr);
    tape.SetHpCutoff(125);
    // init processors
    delay.Init(sr, DELAY_SMOOTH_MS);
    delay.SetFeedbackProcessor(&tape);
    // wow.Init(sr);
    // wow.SetRate(1);
    // wow.SetAmp(.4);
    // flutter.Init(sr);
    // flutter.SetRate(3);
    // flutter.SetAmp(.05);
    wow_osc.Init(sr);
    wow_osc.SetWaveform(Oscillator::WAVE_SIN);
    wow_osc.SetFreq(WOW_FREQ);
    wow_osc.SetAmp(200);
    flutter_osc.Init(sr);
    flutter_osc.SetWaveform(Oscillator::WAVE_SIN);
    flutter_osc.SetFreq(FLUTTER_FREQ);
    flutter_osc.SetAmp(20);  // 50 max
    tempo_osc.Init(sr);
    tempo_osc.SetWaveform(Oscillator::WAVE_SIN);
    tempo_osc.SetAmp(1);

    tapeAttrs.speed = 15;
    tapeAttrs.spacing = .1;
    tapeAttrs.thickness = .1;
    tapeAttrs.gap = 1;
    tapeAttrs.azimuth = 0;

    // init knobs
    time.Init(hw.knob[Terrarium::KNOB_1], MIN_DELAY, MAX_DELAY, Parameter::LOGARITHMIC);
    feedback.Init(hw.knob[Terrarium::KNOB_2], 0.01, 1, Parameter::EXPONENTIAL);
    mix.Init(hw.knob[Terrarium::KNOB_3], 0, 1, Parameter::EXPONENTIAL);
    ageParam.Init(hw.knob[Terrarium::KNOB_4], 0.f, 1.f, Parameter::LINEAR);
    // lpParam.Init(hw.knob[Terrarium::KNOB_5], 400, 15000,
    // Parameter::LOGARITHMIC);
    stabParam.Init(hw.knob[Terrarium::KNOB_5], 0, 1, Parameter::EXPONENTIAL);
    // satParam.Init(hw.knob[Terrarium::KNOB_6], 0, 18, Parameter::LINEAR);

    // wow rate: 0-3hz
    // flutter rate - 0-100 (exp curve)

    hw.StartAdc();
    hw.StartAudio(AudioCallback);

    while(1)
    {
        ProcessSwitches();
        System::Delay(5);
    }
}
