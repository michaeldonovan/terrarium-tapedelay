#include "daisy_petal.h"
#include "daisysp.h"
#include "terrarium.h"
#include "TapeAttrs.hpp"
#include "LossFilter.hpp"
#include "DspUtils.hpp"
#include "Tape.hpp"
#include "Delay.hpp"
// #include "Vibrato.hpp"

using namespace daisy;
using terrarium::Terrarium;
using daisysp::Oscillator;
using daisysp::Svf;


constexpr auto SAMPLE_RATE = 48000;
constexpr auto MIN_DELAY_SEC = 0.1f;
constexpr auto MAX_DELAY_SEC = 1;
constexpr size_t MIN_DELAY = SAMPLE_RATE * MIN_DELAY_SEC;
constexpr size_t MAX_DELAY = SAMPLE_RATE * MAX_DELAY_SEC;

constexpr auto DELAY_SMOOTH_SEC = 0.05f;
constexpr auto DELAY_SMOOTH_COEFF = 1/(DELAY_SMOOTH_SEC * SAMPLE_RATE);

DaisyPetal hw;

bool effectOn = true;
bool tails = true;

Led bypassLed, timeLed;

Parameter mix, time, feedback, lpParam, hpParam, driveParam, satParam, stabParam, ageParam;
float mixVal;


// processors
Delay<MAX_DELAY> delay;
float currDelay;
float targetDelay;
float feedbackVal;
float timeVal;

Tape tape;

TapeAttrs tapeAttrs;

Oscillator wowOsc, flutterOsc;
constexpr float WOW_MAX_AMP = .0083 * SAMPLE_RATE;
constexpr float FLUTTER_MAX_AMP = 0.00077 * SAMPLE_RATE;



Svf hp;
Switch *bypassSw, *tapSw, *tailsSw;

void ProcessControls()
{
	hw.ProcessAllControls();

	if(bypassSw->RisingEdge())
   {
      effectOn = !effectOn;
		bypassLed.Set(effectOn ? 1.f : 0.f);
		if (effectOn && !tails)
		{
			delay.Reset();
		}
   }
	
	mixVal = mix.Process();
	timeVal = time.Process();

	targetDelay = timeVal;
	
	feedbackVal = feedback.Process();

	delay.SetFeedback(feedbackVal);
	delay.SetTime(timeVal);
	delay.EnableInput(effectOn);

	auto stabVal = stabParam.Process();
	wowOsc.SetAmp(WOW_MAX_AMP * stabVal);
	flutterOsc.SetAmp(FLUTTER_MAX_AMP * stabVal);

	// auto hpVal = hpParam.Process();
	// hp.SetFreq(hpVal);

	auto age = ageParam.Process();
   auto lpFc = daisysp::fmap(1-age, 1000.f, 6000.f, daisysp::Mapping::EXP);
	tape.SetLpCutoff(lpFc);
	auto tapeDriveDb = daisysp::fmap(age, 6.f, 24.f, daisysp::Mapping::LINEAR);
	tape.SetDrive(tapeDriveDb);

	
	// timeLed.Set(-1 * tape.GetCompGain());
	timeLed.Set(tails ? 1 : 0);

	bypassLed.Update();
	timeLed.Update();
}
void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	ProcessControls();
	for (size_t i = 0; i < size; i++)
	{
		if (effectOn || tails)
		{
			auto dry = in[0][i];
			auto wet = dry;
			// hp.Process(wet);
			// wet = hp.High();
			// wet = tape.Process(wet);
			wet = delay.Process(wet);

			auto wowVal = wowOsc.Process();
			auto flutterVal = flutterOsc.Process();
			delay.SetTime(timeVal + wowVal + flutterVal);

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
	hw.SetAudioBlockSize(128); 
	hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
	const auto sr = hw.AudioSampleRate();

	bypassSw = &hw.switches[Terrarium::FOOTSWITCH_1];
	tapSw = &hw.switches[Terrarium::FOOTSWITCH_2];
   tailsSw = &hw.switches[Terrarium::SWITCH_1];
	
	// init leds
	bypassLed.Init(hw.seed.GetPin(Terrarium::LED_1), false);
	bypassLed.Update();
	timeLed.Init(hw.seed.GetPin(Terrarium::LED_2), false);
	timeLed.Update();
	
	// init filters
	hp.Init(sr);
	hp.SetRes(.2);
	
	tape.Init(sr);
	tape.SetHpCutoff(125);
	// init processors
	delay.Init(sr, DELAY_SMOOTH_SEC);
	delay.SetFeedbackProcessor(&tape);
	// wow.Init(sr);
	// wow.SetRate(1);
	// wow.SetAmp(.4);
	// flutter.Init(sr);
	// flutter.SetRate(3);
	// flutter.SetAmp(.05);
	wowOsc.Init(sr);
	wowOsc.SetWaveform(Oscillator::WAVE_SIN);
	wowOsc.SetFreq(.7);
	wowOsc.SetAmp(200);
	flutterOsc.Init(sr);
	flutterOsc.SetWaveform(Oscillator::WAVE_SIN);
	flutterOsc.SetFreq(25);
	flutterOsc.SetAmp(20); // 50 max

	tapeAttrs.speed = 15;
	tapeAttrs.spacing = .1;
	tapeAttrs.thickness =  .1;
	tapeAttrs.gap = 1;
	tapeAttrs.azimuth = 0;
	
	// init knobs
	time.Init(hw.knob[Terrarium::KNOB_1], MIN_DELAY, MAX_DELAY, Parameter::LOGARITHMIC);
   feedback.Init(hw.knob[Terrarium::KNOB_2], 0.01, 1, Parameter::EXPONENTIAL);
   mix.Init(hw.knob[Terrarium::KNOB_3], 0, 1, Parameter::EXPONENTIAL);
   ageParam.Init(hw.knob[Terrarium::KNOB_4], 0.f, 1.f, Parameter::LINEAR);
   // lpParam.Init(hw.knob[Terrarium::KNOB_5], 400, 15000, Parameter::LOGARITHMIC);
   stabParam.Init(hw.knob[Terrarium::KNOB_5], 0, 1, Parameter::LINEAR);
	// satParam.Init(hw.knob[Terrarium::KNOB_6], 0, 18, Parameter::LINEAR);

	// wow rate: 0-3hz  
	// flutter rate - 0-100 (exp curve)

	hw.StartAdc();
	hw.StartAudio(AudioCallback);
	
	while(1) { System::Delay(10); }
}
