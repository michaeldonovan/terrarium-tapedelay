#include <memory>
#include <random>

#include <daisysp.h>
#include <daisy_petal.h>

#include <Terrarium/terrarium.h>

#include "mbdsp/Delay.hpp"
#include "mbdsp/TapTempo.hpp"
#include "mbdsp/Tape.hpp"
#include "mbdsp/TapeModels/Echorec.hpp"
#include "mbdsp/TapeModels/SpaceEcho.hpp"
#include "mbdsp/Utils.hpp"

#include "Constants.hpp"
#include "Params.hpp"
// #include "Vibrato.hpp"

using namespace daisy;
using daisysp::Oscillator;
using mbdsp::Delay;
using mbdsp::Tape;
using terrarium::Terrarium;

//////////////////////////////////////////////////////////////////
// Hardware
DaisyPetal hw;

Led bypass_led;
Led tempo_led;

Switch *bypassSw;
Switch *tapSw;
// End hardware
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
// Paramters
Parameter mix, time, feedback, stab_param, age_param, voice_param;

bool effect_enabled = false;
bool tails = true;

bool overload = false;
bool using_tap = false;

// The last time value read from the time knob before switching to tap tempo
float last_time_knob_val;

Params params_active;
Params params_favorite;

mbdsp::SmoothedValue<float> feedback_val;

constexpr uint8_t TAPE_MODELS_SIZE = 3;
const std::shared_ptr<const mbdsp::TapeModel> TAPE_MODELS[TAPE_MODELS_SIZE] = {
    std::make_shared<mbdsp::TapeModel>(),  // default, "Studio" tape
    std::make_shared<mbdsp::TapeModels::SpaceEcho>(),
    std::make_shared<mbdsp::TapeModels::Echorec>()};
// End parameters
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
// processors
Delay<float> delay;

mbdsp::TapTempo<decltype(daisy::System::GetNow())> tap_tempo;

Tape<float> tape;

Oscillator wow_osc;
Oscillator flutter_osc;
Oscillator tempo_osc;
float tempo_osc_val;
// end processors
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
// QSPI
struct FlashData
{
    Params favorite;
    size_t version;
    bool effect_enabled;
};

// QSPI
//////////////////////////////////////////////////////////////////

void FlashSave()
{
    FlashData to_save;
    to_save.version = FLASH_DATA_VERSION;
    to_save.effect_enabled = effect_enabled;
    to_save.favorite = params_favorite;

    hw.seed.qspi.Erase(QSPI_ADDR_BASE, QSPI_ADDR_BASE + sizeof(to_save));
    hw.seed.qspi.Write(QSPI_ADDR_BASE, sizeof(to_save), reinterpret_cast<uint8_t *>(&to_save));
}

void FlashLoad()
{
    const auto flash_data = reinterpret_cast<FlashData *>(hw.seed.qspi.GetData(QSPI_ADDR_BASE));
    if(flash_data->version == FLASH_DATA_VERSION)
    {
        params_favorite = flash_data->favorite;
        effect_enabled = flash_data->effect_enabled;
    }
}

void ProcessSwitches()
{
    hw.ProcessDigitalControls();

    if(bypassSw->RisingEdge())
    {
        effect_enabled = !effect_enabled;
        if(effect_enabled && !tails) { delay.Reset(); }
        FlashSave();
    }
    delay.EnableInput(effect_enabled);

    // if bypass switch held, clear delay
    if(bypassSw->TimeHeldMs() > HOLD_MS) { delay.Reset(); }

    if(tapSw->RisingEdge())
    {
        const auto beat_len_ms = tap_tempo.Tap(daisy::System::GetNow());
        const auto beat_len_samples = beat_len_ms * hw.AudioSampleRate() * .001f;
        if(beat_len_samples >= MIN_DELAY && beat_len_samples <= MAX_DELAY)
        {
            params_active.delay_time = beat_len_samples;
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

    bypass_led.Set(effect_enabled);

    bypass_led.Update();
    tempo_led.Update();
}

void ProcessKnobs()
{
    hw.ProcessAnalogControls();

    params_active.mix = mix.Process();

    const auto time_knob_val = time.Process();

    // Get delay time. Prefer tap tempo if available
    if(using_tap)
    {
        // if we're in tap mode and the time knob has been touched, deactivate
        // tap mode
        if(!mbdsp::within_tolerance(last_time_knob_val, time_knob_val, 5.f))
        {
            using_tap = false;
            params_active.delay_time = time_knob_val;
            tap_tempo.Reset();
        }
    }
    else
    {
        params_active.delay_time = time_knob_val;
        last_time_knob_val = time_knob_val;
    }

    auto fback = feedback.Process();
    if(!overload)
    {
        feedback_val.SetTarget(fback);
        delay.SetTime(params_active.delay_time);
    }

    tempo_osc.SetFreq(hw.AudioSampleRate() / params_active.delay_time);

    auto stab = stab_param.Process();
    params_active.stability = stab;
    wow_osc.SetAmp(WOW_MAX_AMP * stab);
    flutter_osc.SetAmp(FLUTTER_MAX_AMP * stab);

    // adjust wow and flutter times based on tape speed (delay time)
    auto timing_mult = mbdsp::remap(params_active.delay_time, static_cast<float>(MIN_DELAY),
                                    static_cast<float>(MAX_DELAY), -1.f, 1.f);
    auto wow_mod = WOW_FREQ * .2f * timing_mult;
    auto flut_mod = FLUTTER_FREQ * .2f * timing_mult;
    wow_osc.SetFreq(WOW_FREQ + wow_mod);
    flutter_osc.SetFreq(FLUTTER_FREQ + flut_mod);

    const auto tape_model_idx =
        mbdsp::clamp<decltype(TAPE_MODELS_SIZE)>(0, TAPE_MODELS_SIZE, voice_param.Process());
    tape.SetModel(TAPE_MODELS[tape_model_idx]);

    auto age = age_param.Process();
    params_active.age = age;
    tape.SetLossFilter(1 - age);
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    ProcessKnobs();
    for(size_t i = 0; i < size; i++)
    {
        tempo_osc_val = tempo_osc.Process();
        delay.SetFeedback(feedback_val.Process());
        if(effect_enabled || tails)
        {
            auto dry = in[0][i];
            auto wet = dry;
            // wet = tape.Process(wet);
            wet = delay.Process(wet);

            auto wowVal = wow_osc.Process();
            auto flutterVal = flutter_osc.Process();
            delay.SetTime(params_active.delay_time + wowVal + flutterVal);

            // out[0][i] = wet;

            out[0][i] = dry + mbdsp::db_to_amp(params_active.mix) * wet;
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

    // init leds
    bypass_led.Init(hw.seed.GetPin(Terrarium::LED_1), false);
    bypass_led.Update();
    tempo_led.Init(hw.seed.GetPin(Terrarium::LED_2), false);
    tempo_led.Update();
    tap_tempo.Init(MAX_DELAY_SEC * 1000);

    tape.Init(sr);
    tape.SetHpCutoff(150);
    tape.SetDrive(TAPE_DRIVE_DB);
    // init processors
    delay.Init(sr, MAX_DELAY, DELAY_SMOOTH_MS);
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
    // tempo_osc.PhaseAdd(.5);
    tempo_osc.SetAmp(1);

    feedback_val.Init(sr, 250, .5);

    // init knobs
    time.Init(hw.knob[Terrarium::KNOB_1], MIN_DELAY, MAX_DELAY, Parameter::LOGARITHMIC);
    feedback.Init(hw.knob[Terrarium::KNOB_2], 0.f, 1.f, Parameter::LINEAR);
    mix.Init(hw.knob[Terrarium::KNOB_3], -18.f, 0.f, Parameter::LINEAR);
    age_param.Init(hw.knob[Terrarium::KNOB_4], 0.f, 1.f, Parameter::LINEAR);
    // lp_param.Init(hw.knob[Terrarium::KNOB_5], 400, 15000,
    // Parameter::LOGARITHMIC);
    stab_param.Init(hw.knob[Terrarium::KNOB_5], 0.f, 1.f, Parameter::EXPONENTIAL);
    voice_param.Init(hw.knob[Terrarium::KNOB_6], 0, 3, Parameter::LINEAR);

    FlashLoad();

    hw.StartAdc();
    hw.StartAudio(AudioCallback);

    while(1)
    {
        ProcessSwitches();
        System::Delay(5);
    }
}
