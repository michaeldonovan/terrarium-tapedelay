#include "daisy_petal.h"
#include "daisysp.h"
#include "terrarium.h"
#include "TapeAttrs.hpp"
#include "LossFilter.hpp"
#include "DspUtils.hpp"
#include "Tape.hpp"
#include "Delay.hpp"

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

bool effectOn = false;
bool tails = true;

Led bypassLed, timeLed;

Parameter mix, time, feedback, lpParam, hpParam, driveParam, satParam;
float mixVal;


// processors
Delay<MAX_DELAY> delay;
daisysp::DelayLine<float, MAX_DELAY> delayline;
float currDelay;
float targetDelay;
float feedbackVal;

Tape tape;

TapeAttrs tapeAttrs;



Svf lp, hp;
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
	auto timeVal = time.Process();

	targetDelay = timeVal;
	
	feedbackVal = feedback.Process();
	tape.SetGain(satParam.Process());

	delay.SetFeedback(feedbackVal);
	delay.SetTime(timeVal);
	delay.EnableInput(effectOn);

	auto lpVal = lpParam.Process();
	auto hpVal = hpParam.Process();
	lp.SetFreq(lpVal);
	hp.SetFreq(hpVal);

	
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
			lp.Process(wet);
			wet= lp.Low();
			hp.Process(wet);
			wet = hp.High();
			wet = tape.Process(wet);
			wet = delay.Process(wet);

			// daisysp::fonepole(currDelay, targetDelay, 0.0002f);
			// delayline.SetDelay(currDelay);
			// float read = delayline.Read();
			// delayline.Write((feedbackVal * read) + wet);
			// wet = read;




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
	hw.SetAudioBlockSize(4); // number of samples handled per callback
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
	lp.Init(sr);
	lp.SetRes(.1);
	hp.Init(sr);
	hp.SetRes(.2);
	
	tape.Init(sr);
	// init processors
	delay.Init(sr, DELAY_SMOOTH_SEC);
	delayline.Init();

	tapeAttrs.speed = 15;
	tapeAttrs.spacing = .1;
	tapeAttrs.thickness =  .1;
	tapeAttrs.gap = 1;
	tapeAttrs.azimuth = 0;
	
	// init knobs
	time.Init(hw.knob[Terrarium::KNOB_1], MIN_DELAY, MAX_DELAY, Parameter::LINEAR);
   feedback.Init(hw.knob[Terrarium::KNOB_2], 0, 1, Parameter::EXPONENTIAL);
   mix.Init(hw.knob[Terrarium::KNOB_3], 0, 1, Parameter::EXPONENTIAL);
   hpParam.Init(hw.knob[Terrarium::KNOB_4], 0, 400, Parameter::LOGARITHMIC);
   lpParam.Init(hw.knob[Terrarium::KNOB_5], 400, 15000, Parameter::LOGARITHMIC);
	satParam.Init(hw.knob[Terrarium::KNOB_6], 0, 24, Parameter::LINEAR);

	hw.StartAdc();
	hw.StartAudio(AudioCallback);
	
	while(1) { System::Delay(10); }
}
