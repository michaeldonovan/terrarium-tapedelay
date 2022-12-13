#include <random>

#include <daisysp.h>
#include <daisy_petal.h>

#include <Terrarium/terrarium.h>

#include "mbdsp/Delay.hpp"
#include "mbdsp/TapTempo.hpp"
#include "mbdsp/Tape.hpp"
#include "mbdsp/Utils.hpp"

#include "Constants.hpp"
#include "Params.hpp"
#include "TapeAttrs.hpp"
// #include "Vibrato.hpp"

using namespace daisy;
using daisysp::Oscillator;
using mbdsp::Delay;
using mbdsp::Tape;
using terrarium::Terrarium;

DaisyPetal hw;

//////////////////////////////////////////////////////////////////
// Paramters
Parameter mix, time, feedback, stabParam, ageParam;

bool effectOn = false;
bool tails = true;
bool overload = false;
bool using_tap = false;

Params active_params;

mbdsp::SmoothedValue<float> feedback_val;
// End parameters
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
// processors
Delay<MAX_DELAY, float> delay;

mbdsp::TapTempo<decltype(&daisy::System::GetNow)> tap_tempo;

Tape<float> tape;

TapeAttrs tapeAttrs;

Oscillator wow_osc;
Oscillator flutter_osc;
Oscillator tempo_osc;
float tempo_osc_val;
// end processors
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
// Hardware
Led bypass_led;
Led tempo_led;

Switch *bypassSw, *tapSw, *tailsSw;
// End hardware
//////////////////////////////////////////////////////////////////

void ProcessSwitches()
{
    hw.ProcessDigitalControls();

    if(bypassSw->RisingEdge())
    {
        effectOn = !effectOn;
        if(effectOn && !tails) { delay.Reset(); }
    }
    delay.EnableInput(effectOn);

    // if bypass switch held, clear delay
    if(bypassSw->TimeHeldMs() > HOLD_MS) { delay.Reset(); }

    if(tapSw->RisingEdge())
    {
        const auto beat_len_ms = tap_tempo.Tap();
        const auto beat_len_samples = beat_len_ms * hw.AudioSampleRate() * .001f;
        if(beat_len_samples >= MIN_DELAY && beat_len_samples <= MAX_DELAY)
        {
            active_params.delay_time = beat_len_samples;
            using_tap = true;
        }
    }

    // if bypass switch held, clear delay
    if(tapSw->TimeHeldMs() > HOLD_MS)
    {
        overload = true;
        feedback_val.SetTarget(mbdsp::db_to_amp(7));
        tempo_led.Set(1);
    }
    if(tapSw->FallingEdge()) { overload = false; }

    if(!overload) { tempo_led.Set(tempo_osc_val > .95); }

    bypass_led.Set(effectOn);

    bypass_led.Update();
    tempo_led.Update();
}

void ProcessKnobs()
{
    hw.ProcessAnalogControls();

    active_params.mix = mix.Process();

    const auto time_knob_val = time.Process();

    // Get delay time. Prefer tap tempo if available
    if(using_tap)
    {
        // if we're in tap mode and the time knob has been touched, deactivate
        // tap mode
        if(!mbdsp::within_tolerance(active_params.time_knob, time_knob_val, 5.f))
        {
            using_tap = false;
            active_params.delay_time = time_knob_val;
            tap_tempo.Reset();
        }
    }
    else
    {
        active_params.delay_time = time_knob_val;
        active_params.time_knob = time_knob_val;
    }

    auto fback = feedback.Process();
    if(!overload)
    {
        feedback_val.SetTarget(fback);
        delay.SetTime(active_params.delay_time);
    }

    tempo_osc.SetFreq(hw.AudioSampleRate() / active_params.delay_time);

    auto stab = stabParam.Process();
    active_params.stability = stab;
    wow_osc.SetAmp(WOW_MAX_AMP * stab);
    flutter_osc.SetAmp(FLUTTER_MAX_AMP * stab);

    // adjust wow and flutter times based on tape speed (delay time)
    auto timing_mult = mbdsp::remap(active_params.delay_time, static_cast<float>(MIN_DELAY),
                                    static_cast<float>(MAX_DELAY), -1.f, 1.f);
    auto wow_mod = WOW_FREQ * .2f * timing_mult;
    auto flut_mod = FLUTTER_FREQ * .2f * timing_mult;
    wow_osc.SetFreq(WOW_FREQ + wow_mod);
    flutter_osc.SetFreq(FLUTTER_FREQ + flut_mod);

    auto age = ageParam.Process();
    active_params.age = age;
    auto lpFc = mbdsp::remap_exp(1 - age, LOSS_LP_FC_MIN, LOSS_LP_FC_MAX);
    tape.SetLossFilter(lpFc);
    auto tapeDriveDb = mbdsp::remap(age, TAPE_DRIVE_DB_MIN, TAPE_DRIVE_DB_MAX);
    tape.SetDrive(tapeDriveDb);
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    ProcessKnobs();
    for(size_t i = 0; i < size; i++)
    {
        tempo_osc_val = tempo_osc.Process();
        delay.SetFeedback(feedback_val.Process());
        if(effectOn || tails)
        {
            auto dry = in[0][i];
            auto wet = dry;
            // wet = tape.Process(wet);
            wet = delay.Process(wet);

            auto wowVal = wow_osc.Process();
            auto flutterVal = flutter_osc.Process();
            delay.SetTime(active_params.delay_time + wowVal + flutterVal);

            // out[0][i] = wet;

            out[0][i] = dry + active_params.mix * wet;
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
    hw.SetAudioBlockSize(128);
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
    tape.SetHpCutoff(150);
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

    feedback_val.Init(sr, 250, .5);

    tapeAttrs.speed = 15;
    tapeAttrs.spacing = .1;
    tapeAttrs.thickness = .1;
    tapeAttrs.gap = 1;
    tapeAttrs.azimuth = 0;

    // init knobs
    time.Init(hw.knob[Terrarium::KNOB_1], MIN_DELAY, MAX_DELAY, Parameter::LOGARITHMIC);
    feedback.Init(hw.knob[Terrarium::KNOB_2], 0.f, 1.f, Parameter::LINEAR);
    mix.Init(hw.knob[Terrarium::KNOB_3], 0.f, 1.f, Parameter::EXPONENTIAL);
    ageParam.Init(hw.knob[Terrarium::KNOB_4], 0.f, 1.f, Parameter::LINEAR);
    // lpParam.Init(hw.knob[Terrarium::KNOB_5], 400, 15000,
    // Parameter::LOGARITHMIC);
    stabParam.Init(hw.knob[Terrarium::KNOB_5], 0.f, 1.f, Parameter::EXPONENTIAL);
    // satParam.Init(hw.knob[Terrarium::KNOB_6], 0, 18, Parameter::LINEAR);

    hw.StartAdc();
    hw.StartAudio(AudioCallback);

    while(1)
    {
        ProcessSwitches();
        System::Delay(5);
    }
}
