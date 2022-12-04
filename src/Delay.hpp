#pragma once

#include <daisysp.h>
#include "Processor.h"
#include "SmoothedValue.hpp"
template<size_t max_size>
class Delay : public Processor
{
public:
   void Init(float sampleRate, float timeSmoothSec)
   {
      _sr = sampleRate;
      _delay.Init();
      _timeVal.Init(sampleRate, timeSmoothSec);
   }

   float Process(float in) override
   {
      _delay.SetDelay(_timeVal.GetNext());

      auto read = _delay.Read();
      float write = _feedback * read; 
      if (_inputEnable)
      {
         write += in;
      }
      _delay.Write(write);

      return read;
   }

   inline void SetFeedback(float feedback)
   {
      _feedback = feedback;
   }

   inline void EnableInput(bool enable)
   {
      _inputEnable = enable;
   }

   inline void SetTime(float time)
   {
      _timeVal.SetTarget(daisysp::fclamp(time, 0, max_size));
   }

   inline float GetTime()
   {
      return _timeVal.GetCurrent(); 
   }

   inline void Reset()
   {
      _delay.Reset();
   }


private:
   float _sr;
   SmoothedValue _timeVal;
   float _feedback = 0;
   bool _inputEnable = true;
   daisysp::DelayLine<float, max_size> _delay;
};