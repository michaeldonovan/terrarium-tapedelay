#pragma once

#include <daisysp.h>

using daisysp::fonepole;

class SmoothedValue
{
public:
   void Init(float sampleRate, float smoothSeconds = 0.001)
   {
      coeff_ = 1.f/(smoothSeconds * sampleRate);
   }


   inline float GetNext()
   {
      fonepole(curr_, target_, coeff_);
      return curr_;
   }

   inline void SetTarget(float target)
   {
      target_ = target;
   }

   inline float GetCurrent()
   {
      return curr_;
   }


private:
   float target_;
   float curr_;
   float coeff_;

};