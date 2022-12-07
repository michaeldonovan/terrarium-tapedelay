#ifndef DSPUTILS_HPP
#define DSPUTILS_HPP

#include <math.h>
#include <daisysp.h>

namespace mbdsp
{

constexpr double pi() { return std::atan(1)*4; } 
constexpr double twopi() { return pi() * 2; }

constexpr auto MS_PER_SEC = 1000;
constexpr auto SEC_PER_MIN= 60;


/**
 * From https://openaudio.blogspot.com/2017/02/faster-log10-and-pow.html
 */
template<class Float_t>
inline Float_t fast_pow10(Float_t x)
{
   return std::exp(2.302585092994046 * x);
}


/**
 * From https://openaudio.blogspot.com/2017/02/faster-log10-and-pow.html
 */
template<class Float_t>
inline Float_t log2_approx(Float_t x)
{
   Float_t Y, F;
   int E;
   F = std::frexp(std::fabs(x), &E);
   Y = 1.23149591368684f;
   Y *= F;
   Y += -4.11852516267426f;
   Y *= F;
   Y += 6.02197014179219f;
   Y *= F;
   Y += -3.13396450166353f;
   Y += E;
   return Y;
}

/**
 * From https://openaudio.blogspot.com/2017/02/faster-log10-and-pow.html
 */
template<class Float_t>
inline Float_t log10_approx(Float_t x)
{
   return log2_approx(x) * 0.3010299956639812;
}

template<class Float_t>
inline Float_t amp_to_db(Float_t amp)
{
   return log10_approx(amp) * 20.f;
}

template<class Float_t>
inline Float_t db_to_amp(Float_t db)	
{
	return fast_pow10(db / 20.f);
}

/**
 * Fast power approximation.
 * 
 * Originally Stefan Stenzel, from https://www.musicdsp.org/en/latest/Other/133-fast-power-and-root-estimates-for-32bit-floats.html
 */
inline float powf_approx(float f, int n)
{
   auto* lp = reinterpret_cast<long*>(&f);
   auto l = *lp;
   l -= 0x3F800000l;
   l <<= (n-1);
   l += 0x3F800000l;
   *lp=l;
   return f;
}

/**
 * Fast root approximation.
 * 
 * Originally Stefan Stenzel, from https://www.musicdsp.org/en/latest/Other/133-fast-power-and-root-estimates-for-32bit-floats.html
 */
inline float root_approx(float f, int n)
{
   auto* lp = reinterpret_cast<long*>(&f);
   auto l = *lp;
   l-=0x3F800000l;
   l>>=(n-1);
   l+=0x3F800000l;
   *lp=l;
   return f;
}

/**
 * By uh.etle.fni@yfoocs from https://www.musicdsp.org/en/latest/Other/222-fast-exp-approximations.html
 */
inline constexpr float fastexp3(float x) {
    return (6+x*(6+x*(3+x)))*0.16666666f;
}

/**
 * By uh.etle.fni@yfoocs from https://www.musicdsp.org/en/latest/Other/222-fast-exp-approximations.html
 */
inline constexpr float fastexp4(float x) {
    return (24+x*(24+x*(12+x*(4+x))))*0.041666666f;
}

/**
 * By uh.etle.fni@yfoocs from https://www.musicdsp.org/en/latest/Other/222-fast-exp-approximations.html
 */
inline constexpr float fastexp5(float x) {
    return (120+x*(120+x*(60+x*(20+x*(5+x)))))*0.0083333333f;
}

/**
 * By uh.etle.fni@yfoocs from https://www.musicdsp.org/en/latest/Other/222-fast-exp-approximations.html
 */
inline constexpr float fastexp6(float x) {
    return 720+x*(720+x*(360+x*(120+x*(30+x*(6+x)))))*0.0013888888f;
    }

/**
 * By uh.etle.fni@yfoocs from https://www.musicdsp.org/en/latest/Other/222-fast-exp-approximations.html
 */
inline constexpr float fastexp7(float x) {
    return (5040+x*(5040+x*(2520+x*(840+x*(210+x*(42+x*(7+x)))))))*0.00019841269f;
}

/**
 * By uh.etle.fni@yfoocs from https://www.musicdsp.org/en/latest/Other/222-fast-exp-approximations.html
 */
inline constexpr float fastexp8(float x) {
    return (40320+x*(40320+x*(20160+x*(6720+x*(1680+x*(336+x*(56+x*(8+x))))))))*2.4801587301e-5;
}

/**
 * By uh.etle.fni@yfoocs from https://www.musicdsp.org/en/latest/Other/222-fast-exp-approximations.html
 */
inline constexpr float fastexp9(float x) {
  return (362880+x*(362880+x*(181440+x*(60480+x*(15120+x*(3024+x*(504+x*(72+x*(9+x)))))))))*2.75573192e-6;
}

/**
 * Exp approximation for values in the range [0, 7.5]. Max error ~0.45%
 * 
 * By uh.etle.fni@yfoocs from https://www.musicdsp.org/en/latest/Other/222-fast-exp-approximations.html
 */
inline constexpr float fastexp(float x)
{
   if (x<2.5)
      return 2.7182818f * fastexp5(x-1.f);
   else if (x<5)
      return 33.115452f * fastexp5(x-3.5f);
   else
      return 403.42879f * fastexp5(x-6.f);
}

/**
 * pow2 approxmiation for values in the range [0, 10.58].l Max error ~0.45%
 *
 * By uh.etle.fni@yfoocs from https://www.musicdsp.org/en/latest/Other/222-fast-exp-approximations.html
 */
inline constexpr float fastpow2(float x)
{
  float const log_two = 0.6931472f;
  return fastexp(x * log_two);
}


template <class Numeric_t>
inline constexpr Numeric_t clamp(Numeric_t in, Numeric_t min, Numeric_t max)
{
   return std::min(std::max(in, min), max);
}

/**
 * @brief Remaps an input value in the range [0, 1] to range [min, max]
 */
template <class Numeric_t>
inline constexpr Numeric_t remap(Numeric_t in, Numeric_t min, Numeric_t max) 
{
   return min + in * (max - min);
}

/**
 * @brief Remaps an input value in the range [0, 1] to range [min, max]
 */
template <class Numeric_t>
inline constexpr Numeric_t remap_exp(Numeric_t in, Numeric_t min, Numeric_t max) 
{
   return remap(in*in, min, max);
}

/**
 * @brief Remaps an input value in the range [old_min, old_max] to range [new_min, new_max]
 */
template <class Numeric_t>
inline constexpr Numeric_t remap(Numeric_t in, Numeric_t old_min, Numeric_t old_max, Numeric_t new_min, Numeric_t new_max) 
{
   return (in - old_min) / (old_max - old_min) * (new_max - new_min) + new_min;
}

/**
 * @brief Remaps an input value in the range [old_min, old_max] to range [new_min, new_max]
 */
template <class Numeric_t>
inline constexpr Numeric_t remap_exp(Numeric_t in, Numeric_t old_min, Numeric_t old_max, Numeric_t new_min, Numeric_t new_max) 
{
   return remap(in*in, new_min, new_max, old_min, old_max);
}

//https://stackoverflow.com/questions/1903954/is-there-a-standard-sign-function-signum-sgn-in-c-c#4609795
template <class Numeric_t>
inline constexpr int sign(Numeric_t val)
{
   return (static_cast<Numeric_t>(0) < val) - (val < static_cast<Numeric_t>(0));
}


template <class Numeric_t>
inline constexpr Numeric_t MsToBpm(Numeric_t ms)
{
   return ms / static_cast<Numeric_t>(MS_PER_SEC * SEC_PER_MIN);
}


template <class Numeric_t>
inline constexpr bool WithinTolerance(Numeric_t target, Numeric_t val, Numeric_t tolerance_percent = 5)
{
   const auto tolerance = target * tolerance_percent * .01;
   return val >= target - tolerance && val <= target + tolerance;
}

/**
 * DC blocker
 * 
 * Based on code by hc.niweulb@lossor.ydna, which was based on code by Julius O. Smith III.
 * 
 * From https://www.musicdsp.org/en/latest/Filters/135-dc-filter.html?highlight=dc
 */
template <class Float_t>
class dc_blocker
{
public:
   dc_blocker(Float_t sample_rate, Float_t freq_hz = 20)
   : fs_(sample_rate)
   {
      R_ = 1 - (pi() * 2 * freq_hz / fs_);
   }
   dc_blocker(const dc_blocker&) = delete;
   dc_blocker operator=(const dc_blocker&) = delete;
   dc_blocker(dc_blocker&&) = default;
   dc_blocker operator=(dc_blocker&&) = default;
   ~dc_blocker() = default;

   inline Float_t dc_filter(Float_t in, Float_t sample_rate)
   {
      auto out = in - last_in_ + R_ * last_out_;
      last_in_ = in;
      last_out_ = out;
      return out; 
   }

private:
   const Float_t fs_;
   const Float_t R_;
   Float_t last_in_ = 0;
   Float_t last_out_ = 0;
};
}

#endif // DSPUTILS_HPP