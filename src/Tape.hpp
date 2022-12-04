#pragma once
#include <daisysp.h>

#include "DspUtils.hpp"
#include "Processor.h"
#include "HysteresisProcessing.h"
#include "EnvelopeFollower.hpp"

class Tape : public Processor
{
public:
   void Init(float sampleRate)
   {
      _hyst.setSampleRate(sampleRate);
      _hyst.cook(.5, .5, .5, false);
      _hyst.reset();
      // _comp.Init(sampleRate);
      // _comp.SetAttack(0.003); // 3ms
      // _comp.SetRelease(0.15); // 150ms 
      // _comp.SetRatio(1.1);
      // _comp.SetThreshold(-45);
      // _comp.AutoMakeup(true);
      _comp.init(3, 150, 0, 1.1, 1, sampleRate);
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

      auto wet = in;
      wet = _comp.process(wet);
      wet *= _gain;

      // clip input to avoid unstable hysteresis
      wet = daisysp::fclamp(wet, -10, 10);
      wet = _hyst.process<RK4>((double)wet);
      wet /= _gain;
      _lp.Process(wet);
      wet = _lp.Low();
      _hp.Process(wet);
      wet = _hp.High();

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
      return 0;
      // return _comp.GetGain();
   }

private: 
   HysteresisProcessing _hyst;
   // daisysp::Compressor _comp;
   Dynamics _comp;
   daisysp::Svf _lp;
   daisysp::Svf _hp;
   float _gain;
};