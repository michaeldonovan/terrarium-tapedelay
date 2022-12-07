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
      os_rate_ = sampleRate*2;
      hyst_.setSampleRate(os_rate_);
      hyst_.cook(.5, .5, .5, false);
      hyst_.reset();
      os_.Init();
      // comp_.Init(sampleRate);
      // comp_.SetAttack(0.003); // 3ms
      // comp_.SetRelease(0.15); // 150ms 
      // comp_.SetRatio(1.1);
      // comp_.SetThreshold(-45);
      // comp_.AutoMakeup(true);
      comp_.init(3, 150, 0, 1.1, 3, os_rate_);
      comp_.setMode(Dynamics::COMP);
      comp_.setThreshold(-45);
      lp_.Init(os_rate_);
      lp_.SetFreq(5000);
      lp_.SetRes(0);
      hp_.Init(os_rate_);
      hp_.SetFreq(50);
      hp_.SetRes(0);
   }
   
   float Process(float in) override
   {
      auto sample = os_.Process(in, [this](float sample){

         // Compression
         sample = comp_.process(sample);

         // Hysteresis
         sample *= pre_gain_;
         // clip input to avoid unstable hysteresis
         sample = mbdsp::clamp<float>(sample, -10, 10);
         sample = hyst_.process<RK2>((double)sample);
         sample /= pre_gain_;

         // Loss
         lp_.Process(sample);
         sample = lp_.Low();
         hp_.Process(sample);
         sample = hp_.High();

         return sample;
      });

      return sample * post_gain_;
   }

   inline void SetDrive(float gainDb)
   {
      pre_gain_ = mbdsp::db_to_amp(gainDb);
   }

   inline float GetCompGain()
   {
      return 0;
      // return comp_.GetGain();
   }

   inline void SetLossFilter(float freq)
   {
      static constexpr auto min_fc = 1000.f;
      static constexpr auto max_fc = 15000.f;

      freq = mbdsp::clamp(freq, min_fc, max_fc);
      lp_.SetFreq(freq);
      
      // add a bit of gain to compensate for loudness lost due to filter
      auto gain_db = mbdsp::remap(max_fc - freq, min_fc, max_fc, 0.f, 6.f);
      post_gain_ = mbdsp::db_to_amp(gain_db);
   }

   inline void SetHpCutoff(float freq)
   {
      hp_.SetFreq(freq);
   }


private: 
   Oversampler os_;
   HysteresisProcessing hyst_;
   // daisysp::Compressor comp_;
   Dynamics comp_;
   daisysp::Svf lp_;
   daisysp::Svf hp_;
   float post_gain_;
   float pre_gain_;
   float os_rate_;
};