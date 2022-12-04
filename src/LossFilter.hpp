#pragma once
#include <daisysp.h>
#include "TapeAttrs.hpp"

class LossFilter
{
public:
   void Init(float sampleRate, TapeAttrs* tapeAttrs)
   {
      _sr = sampleRate;
      _attrs = tapeAttrs;
      _bump.Init(sampleRate);
   }
   
   void SetTapeAttrs(TapeAttrs* tapeAttrs)
   {
      _prevAttrs = *_attrs;
      _attrs = tapeAttrs;
   }
   
   float Process(float in)
   {
      // if (*_attrs != _prevAttrs)
      // {
      //    calcCoefs(bumpFilter)
      // }
      
      
   }

   
      
private:

   inline float gapMeters()
   {
      return _attrs->gap * (float)1.0e-6;
      
   }

   void CalcHeadBumpFilter()
   {
      // _bump.SetFreq(_attrs->speed * 0.0254f / (gapMeters()*500));
      // _bump.S 
      
   }


   TapeAttrs* _attrs;
   TapeAttrs _prevAttrs;
   float _sr;
   daisysp::Svf _bump;
};