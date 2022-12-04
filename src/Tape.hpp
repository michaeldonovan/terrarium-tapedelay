#pragma once
#include <daisysp.h>

#include "DspUtils.hpp"
#include "Processor.h"
#include "HysteresisProcessing.h"
#include "EnvelopeFollower.hpp"
#include "hiir/Upsampler2xFpu.h"
#include "hiir/Downsampler2xFpu.h"
#include "hiir/PolyphaseIir2Designer.h"

constexpr size_t TAPE_RESAMPLE_N_COEFFS = 8;

class Tape : public Processor
{
public:
   void Init(float sampleRate)
   {
      hiir::PolyphaseIir2Designer::compute_coefs_spec_order_tbw(_resampleCoefs, TAPE_RESAMPLE_N_COEFFS, 0.04);
      _upsampler.set_coefs(_resampleCoefs);
      _downsampler.set_coefs(_resampleCoefs);
      auto osRate = sampleRate*2;
      _hyst.setSampleRate(osRate);
      _hyst.cook(.5, .5, .5, false);
      _hyst.reset();
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
      // Begin oversampling
      float upsampled[2] = {in, 0};
      _upsampler.process_sample(upsampled[0], upsampled[1], in);

      for (size_t i = 0; i < 2; ++i)
      {
         auto sample = upsampled[i];
         sample = _comp.process(sample);
         sample *= _gain;


         // clip input to avoid unstable hysteresis
         sample = daisysp::fclamp(sample, -10, 10);
         sample = _hyst.process<RK4>((double)sample);
         sample /= _gain;

         upsampled[i] = sample;
      }
      auto sample = _downsampler.process_sample(upsampled);
      // End oversampling

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
   hiir::Upsampler2xFpu<TAPE_RESAMPLE_N_COEFFS> _upsampler;
   hiir::Downsampler2xFpu<TAPE_RESAMPLE_N_COEFFS> _downsampler;
   double _resampleCoefs[TAPE_RESAMPLE_N_COEFFS];
   HysteresisProcessing _hyst;
   // daisysp::Compressor _comp;
   Dynamics _comp;
   daisysp::Svf _lp;
   daisysp::Svf _hp;
   float _gain;
};