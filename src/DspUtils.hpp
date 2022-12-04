#pragma once
#include <math.h>
#include <daisysp.h>

inline float qToRes(float q)
{
   if (q == 0.f)
      return 0.f;
   return 1 - (1/(2.f*q));
}

inline float ampToDb(float x)
{
   return daisysp::fastlog10f(x) * 20.f;
}

inline float dbToAmp(float db)	
{
	return daisysp::pow10f(db / 20.f);
}
