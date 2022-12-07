#pragma once

#include <daisysp.h>
#include "Processor.h"
#include "SmoothedValue.hpp"
#include "EnvelopeFollower.hpp"

template<size_t max_size>
class Delay : public Processor
{
public:
   void Init(float sample_rate, float time_smooth_ms)
   {
      fs_ = sample_rate;
      delay_.Init();
      timeVal_.Init(sample_rate, time_smooth_ms);
   }

   float Process(float in) override
   {
      delay_.SetDelay(timeVal_.Process());

      auto read = delay_.Read();
      float write = feedback_ * read; 
      if (inputEnable_)
      {
         write += in;
      }

      if (proc_)
      {
         write = proc_->Process(write);
      }

      delay_.Write(write);

      return read;
   }

   inline void SetFeedback(float feedback)
   {
      feedback_ = feedback;
   }

   inline void EnableInput(bool enable)
   {
      inputEnable_ = enable;
   }

   inline void SetTime(float time)
   {
      timeVal_.SetTarget(mbdsp::clamp<float>(time, 0, max_size));
   }

   inline float GetTime()
   {
      return timeVal_.GetCurrent(); 
   }

   inline void Reset()
   {
      delay_.Reset();
   }

   inline void SetFeedbackProcessor(Processor* proc)
   {
      proc_ = proc;
   }

private:
   float fs_;
   mbdsp::SmoothedValue<float> timeVal_;
   float feedback_ = 0;
   bool inputEnable_ = true;
   Processor* proc_;
   daisysp::DelayLine<float, max_size> delay_;
};