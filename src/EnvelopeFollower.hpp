//
//  EnvFollower.h
//
//
//
//

#ifndef EnvFollower_h
#define EnvFollower_h

#include <daisysp.h>

#include <cmath>
#include <vector>

#include "DspUtils.hpp"

class EnvFollower
{
public:
    enum kMode
    {
        kPeak,
        kRMS
    };

    EnvFollower() { init(kPeak, 5, 50, 0, 44100); }

    EnvFollower(float attackMS, float releaseMS, float holdMS, float SampleRate)
    {
        init(kPeak, attackMS, releaseMS, holdMS, SampleRate);
    }

    virtual void init(int detectMode, float attackMS, float releaseMS, float holdMS,
                      float SampleRate)
    {
        mode = detectMode;
        fs = SampleRate;
        attack = mbdsp::powf_approx(0.01, 1.0 / (attackMS * fs * 0.001));
        release = mbdsp::powf_approx(0.01, 1.0 / (releaseMS * fs * 0.001));
        hold = holdMS / 1000. * fs;
        env = 0;
        timer = 0;
        rmsWindowLength = SampleRate * 0.2;
        buffer.resize(rmsWindowLength);
        index = 0;
    }

    void setAttack(float attackMS) { attack = attackMS; }

    void setRelease(float releaseMS) { release = releaseMS; }

    void setHold(float holdMS) { hold = holdMS; }

    void setDetectMode(int detectorMode) { mode = detectorMode; }

    virtual float process(float sample)
    {
        float mag;
        if(mode == kRMS) {}
        else { mag = fabs(sample); }
        if(mag > env)
        {
            env = attack * (env - mag) + mag;
            timer = 0;
        }
        else if(timer < hold) { timer++; }
        else { env = release * (env - mag) + mag; }

        return env;
    }

protected:
    float attack, release, env, fs;
    int index, timer, hold, mode, rmsWindowLength;
    std::vector<float> buffer;
};

class Dynamics : public EnvFollower
{
public:
    enum Mode
    {
        COMP,
        LIMIT
    };

    Dynamics(){};

    Dynamics(float attackMS, float releaseMS, float holdMS, float ratio, float knee,
             float SampleRate)
    {
        init(attackMS, releaseMS, holdMS, ratio, knee, SampleRate);
    }

    void init(float attackMS, float releaseMS, float holdMS, float ratio, float knee,
              float SampleRate)
    {
        EnvFollower::init(kPeak, attackMS, releaseMS, holdMS, SampleRate);
        mCompMode = 0;
        gainReduction = 0;
        mKnee = knee;
        mRatio = ratio;
        mThreshold = 0.;
        calcKnee();
        calcSlope();
    }

    void setAttack(float attackMS)
    {
        attack = daisysp::fastpower(0.01, 1.0 / (attackMS * fs * 0.001));
    }

    void setRelease(float releaseMS)
    {
        release = daisysp::fastpower(0.01, 1.0 / (releaseMS * fs * 0.001));
    }

    void setHold(float holdMS) { hold = holdMS / 1000. * fs; }

    void setKnee(float knee)
    {
        mKnee = knee;
        calcKnee();
        calcSlope();
    }

    void setRatio(float ratio)
    {
        mRatio = ratio;
        calcKnee();
        calcSlope();
    }

    void setThreshold(float thresholdDB)
    {
        mThreshold = thresholdDB;
        calcKnee();
        calcSlope();
    }

    void setMode(int mode)
    {
        mCompMode = mode;
        calcSlope();
    }

    float getThreshold() { return mThreshold; }
    float getAttack() { return attack; }
    float getRelease() { return release; }
    float getHold() { return hold; }
    float getKnee() { return mKnee; }
    float getRatio() { return mRatio; }
    float getGainReductionDB() { return gainReduction; }
    float getKneeBoundL() { return kneeBoundL; }
    float getKneeBoundU() { return kneeBoundU; }

    inline float process(float sample)
    {
        float e = mbdsp::amp_to_db(EnvFollower::process(sample));
        if(kneeWidth > 0.f && e > kneeBoundL && e < kneeBoundU)
        {
            slope *= ((e - kneeBoundL) / kneeWidth) * 0.5;
            gainReduction = slope * (kneeBoundL - e);
        }
        else
        {
            gainReduction = slope * (mThreshold - e);
            gainReduction = fmin(0., gainReduction);
        }

        return sample * mbdsp::db_to_amp(gainReduction);
    }

    // Takes in two samples, processes them, and returns gain reduction in dB
    float processStereo(float sample1, float sample2)
    {
        float e = mbdsp::amp_to_db(EnvFollower::process(fmax(sample1, sample2)));
        calcSlope();

        if(kneeWidth > 0. && e > kneeBoundL && e < kneeBoundU)
        {
            slope *= ((e - kneeBoundL) / kneeWidth) * 0.5;
            gainReduction = slope * (kneeBoundL - e);
        }
        else
        {
            gainReduction = slope * (mThreshold - e);
            gainReduction = fmin(0., gainReduction);
        }

        return gainReduction;
    }

private:
    float gainReduction, mKnee, mRatio, mThreshold, kneeWidth, kneeBoundL, kneeBoundU, slope;
    int mCompMode;

    inline void calcKnee()
    {
        kneeWidth = mThreshold * mKnee * -1.;
        kneeBoundL = mThreshold - (kneeWidth / 2.);
        kneeBoundU = mThreshold + (kneeWidth / 2.);
    }

    inline void calcSlope()
    {
        if(mCompMode == COMP) { slope = 1 - (1 / mRatio); }
        else if(mCompMode == LIMIT) { slope = 1; }
    }
};
#endif /* EnvFollower_h */
