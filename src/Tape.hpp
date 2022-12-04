#pragma once
#include <daisysp.h>

#include "DspUtils.hpp"
#include "Processor.h"

class Tape : public Processor
{
public:
   void Init(float sampleRate)
   {
      _comp.Init(sampleRate);
      _comp.SetAttack(0.005); // 1ms
      _comp.SetRelease(0.15); // 150ms
      _comp.SetRatio(1.1);
      _comp.SetThreshold(-45);
      _comp.AutoMakeup(true);
   }
   
   float Process(float in) override
   {
      auto wet = _comp.Process(in);
      wet *= _gain;
      wet = daisysp::SoftClip(wet);
      wet /= _gain;
      return wet;
   }

   inline void SetGain(float gainDb)
   {
      _gain = dbToAmp(gainDb);
   }

   inline float GetCompGain()
   {
      return _comp.GetGain();
   }

   inline float GetGain()
   {
      return ampToDb(_gain);
   }

private: 
   daisysp::Compressor _comp;
   float _gain = 1;
};