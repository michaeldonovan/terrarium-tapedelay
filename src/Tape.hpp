#pragma once
#include <daisysp.h>

#include "DspUtils.hpp"
#include "Processor.h"
#include "HysteresisProcessing.h"

class Tape : public Processor
{
public:
   void Init(float sampleRate)
   {
      _hyst.setSampleRate(sampleRate);
      _hyst.cook(.5, .5, .5, false);
      _hyst.reset();
      _comp.Init(sampleRate);
      _comp.SetAttack(0.001); // 1ms
      _comp.SetRelease(0.15); // 150ms
      _comp.SetRatio(1.1);
      _comp.SetThreshold(-45);
      _comp.AutoMakeup(true);
   }
   
   float Process(float in) override
   {
      auto wet = _comp.Process(in);
      wet *= _gain;

      // clip input to avoid unstable hysteresis
      wet = daisysp::fclamp(wet, -10, 10);
      wet = _hyst.process<RK4>((double)wet);


      wet /= _gain;

      // wet = daisysp::soft_saturate(wet, dbToAmp(-35) );
      // wet = daisysp::SoftClip(wet);
      return wet;
   }

   inline void SetGain(float gainDb)
   {
      // _gain = dbToAmp(gainDb);
      _gain = gainDb;
   }

   inline float GetCompGain()
   {
      return _comp.GetGain();
   }

private: 
   HysteresisProcessing _hyst;
   daisysp::Compressor _comp;
   float _gain;
};