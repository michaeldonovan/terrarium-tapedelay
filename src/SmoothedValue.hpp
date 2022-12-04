#pragma once

#include <daisysp.h>

using daisysp::fonepole;

class SmoothedValue
{
public:
   void Init(float sampleRate, float smoothSeconds = 0.001)
   {
      _coeff = 1.f/(smoothSeconds * sampleRate);
   }


   inline float GetNext()
   {
      fonepole(_curr, _target, _coeff);
      return _curr;
   }

   inline void SetTarget(float target)
   {
      _target = target;
   }

   inline float GetCurrent()
   {
      return _curr;
   }


private:
   float _target;
   float _curr;
   float _coeff;

};