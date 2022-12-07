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
      upsampler_2x.set_coefs(coeffs2x);
      upsampler_2x.clear_buffers();
      downsampler_2x.set_coefs(coeffs2x);
      downsampler_2x.clear_buffers();
      upsampler_4x.set_coefs(coeffs4x);
      upsampler_4x.clear_buffers();
      downsampler_4x.set_coefs(coeffs4x);
      downsampler_4x.clear_buffers();
   }

   float Process(float in, std::function<float(float)> proc)
   {
      float upBuff2x[2];
      float upBuff4x[4];
      float downBuff4x[4];
      float downBuff2x[2];

      upsampler_2x.process_sample(upBuff2x[0], upBuff2x[1], in);
      // upsampler_4x.process_block(upBuff4x, upBuff2x, 2);
      downBuff2x[0] = proc(upBuff2x[0]);
      downBuff2x[1] = proc(upBuff2x[1]);

      // for (size_t i = 0; i < 4; ++i)
      // {
      //    downBuff4x[i] = proc(upBuff4x[i]);
      // }

      // downsampler_4x.process_block(downBuff2x, downBuff4x, 4);

      return downsampler_2x.process_sample(downBuff2x);
   }



private:
   // coefficients from hiir examples in oversampling.txt

   hiir::Upsampler2xFpu<N_COEFFS_2X> upsampler_2x;
   hiir::Downsampler2xFpu<N_COEFFS_2X> downsampler_2x;
   hiir::Upsampler2xFpu<N_COEFFS_4X> upsampler_4x;
   hiir::Downsampler2xFpu<N_COEFFS_4X> downsampler_4x;


};