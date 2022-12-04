#pragma once

#include <daisysp.h>
#include "Processor.h"

template<size_t max_size>
class Delay : public Processor
{
public:
   void Init(float sampleRate, float timeSmoothSec)
   {
      _sr = sampleRate;
      _delay.Init();
      _timeSmoothCoeff = 1/(timeSmoothSec * sampleRate);
   }

   float Process(float in) override
   {
      daisysp::fonepole(_currTime, _targetTime, _timeSmoothCoeff);
      _delay.SetDelay(_currTime);

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
      _targetTime = time;
   }

   inline float GetTime()
   {
      return _currTime;
   }

   inline void Reset()
   {
      _delay.Reset();
   }


private:
   float _sr;
   float _timeSmoothCoeff = 0;
   float _currTime = 1000;
   float _targetTime = 1000;
   float _feedback = 0;
   bool _inputEnable = true;
   daisysp::DelayLine<float, max_size> _delay;
};