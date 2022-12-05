#pragma once
#include <functional>
#include "hiir/Upsampler2xFpu.h"
#include "hiir/Downsampler2xFpu.h"
#include "hiir/PolyPHaseIir2Designer.h"

class Oversampler
{
public:
   static constexpr auto N_COEFFS_2X = 12;
   static constexpr auto N_COEFFS_4X = 6;
   void Init()
   {
      double coeffs2x[N_COEFFS_2X];
      double coeffs4x[N_COEFFS_4X];
      hiir::PolyphaseIir2Designer::compute_coefs_spec_order_tbw(coeffs2x, N_COEFFS_2X, 0.01);
      hiir::PolyphaseIir2Designer::compute_coefs_spec_order_tbw(coeffs4x, N_COEFFS_4X, 0.01);
      _upsampler2x.set_coefs(coeffs2x);
      _upsampler2x.clear_buffers();
      _downsampler2x.set_coefs(coeffs2x);
      _downsampler2x.clear_buffers();
      _upsampler4x.set_coefs(coeffs4x);
      _upsampler4x.clear_buffers();
      _downsampler4x.set_coefs(coeffs4x);
      _downsampler4x.clear_buffers();
   }

   float Process(float in, std::function<float(float)> proc)
   {
      float upBuff2x[2];
      float upBuff4x[4];
      float downBuff4x[4];
      float downBuff2x[2];

      _upsampler2x.process_sample(upBuff2x[0], upBuff2x[1], in);
      // _upsampler4x.process_block(upBuff4x, upBuff2x, 2);
      for (size_t i = 0; i < 2; ++i)
      {
         downBuff2x[i] = proc(upBuff2x[i]);
      }

      // for (size_t i = 0; i < 4; ++i)
      // {
      //    downBuff4x[i] = proc(upBuff4x[i]);
      // }

      // _downsampler4x.process_block(downBuff2x, downBuff4x, 4);

      return _downsampler2x.process_sample(downBuff2x);
   }



private:
   // coefficients from hiir examples in oversampling.txt

   hiir::Upsampler2xFpu<N_COEFFS_2X> _upsampler2x;
   hiir::Downsampler2xFpu<N_COEFFS_2X> _downsampler2x;
   hiir::Upsampler2xFpu<N_COEFFS_4X> _upsampler4x;
   hiir::Downsampler2xFpu<N_COEFFS_4X> _downsampler4x;


};