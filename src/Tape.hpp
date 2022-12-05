#pragma once
#include <daisysp.h>

#include "DspUtils.hpp"
#include "Processor.h"
#include "HysteresisProcessing.h"
#include "EnvelopeFollower.hpp"
#include "Oversampler.hpp"

class Tape : public Processor
{
public:
   void Init(float sampleRate)
   {
      auto osRate = sampleRate*2;
      _hyst.setSampleRate(osRate);
      _hyst.cook(.5, .5, .5, false);
      _hyst.reset();
      _os.Init();
      // _comp.Init(sampleRate);
      // _comp.SetAttack(0.003); // 3ms
      // _comp.SetRelease(0.15); // 150ms 
      // _comp.SetRatio(1.1);
      // _comp.SetThreshold(-45);
      // _comp.AutoMakeup(true);
      _comp.init(3, 150, 0, 1.1, 3, osRate);
      _comp.setMode(Dynamics::COMP);
      _comp.setThreshold(-45);
      _lp.Init(sampleRate);
      _lp.SetFreq(5000);
      _lp.SetRes(0);
      _hp.Init(sampleRate);
      _hp.SetFreq(50);
      _hp.SetRes(0);
   }
   
   float Process(float in) override
   {
      auto sample = _os.Process(in, [this](float sample){
         sample = _comp.process(sample);
         sample *= _gain;


         // clip input to avoid unstable hysteresis
         sample = daisysp::fclamp(sample, -10, 10);
         sample = _hyst.process<RK4>((double)sample);
         sample /= _gain;
         return sample;
      });

      _lp.Process(sample);
      sample = _lp.Low();
      _hp.Process(sample);
      sample = _hp.High();
      return sample;
   }

   inline void SetDrive(float gainDb)
   {
      _gain = dbToAmp(gainDb);
   }

   inline float GetCompGain()
   {
      return 0;
      // return _comp.GetGain();
   }

   inline void SetLpCutoff(float freq)
   {
      _lp.SetFreq(freq);
   }

   inline void SetHpCutoff(float freq)
   {
      _hp.SetFreq(freq);
   }


private: 
   Oversampler _os;
   HysteresisProcessing _hyst;
   // daisysp::Compressor _comp;
   Dynamics _comp;
   daisysp::Svf _lp;
   daisysp::Svf _hp;
   float _gain;
};