#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <daisy.h>

constexpr uint8_t FLASH_DATA_VERSION = 1;

constexpr size_t QSPI_ADDR_BASE = 0;

constexpr auto DAISY_SR = daisy::SaiHandle::Config::SampleRate::SAI_48KHZ;

constexpr float SampleRate()
{
    switch(DAISY_SR)
    {
        case daisy::SaiHandle::Config::SampleRate::SAI_96KHZ:
            return 96000;
            break;
        case daisy::SaiHandle::Config::SampleRate::SAI_48KHZ:
            return 48000;
            break;
        case daisy::SaiHandle::Config::SampleRate::SAI_32KHZ:
            return 32000;
            break;
        case daisy::SaiHandle::Config::SampleRate::SAI_16KHZ:
            return 16000;
            break;
        case daisy::SaiHandle::Config::SampleRate::SAI_8KHZ:
            return 8000;
            break;
    }
};

constexpr auto HOLD_MS = 250;

// delay params
constexpr auto MIN_DELAY_SEC = 0.1f;
constexpr auto MAX_DELAY_SEC = 1.f;
constexpr size_t MIN_DELAY = SampleRate() * MIN_DELAY_SEC;
constexpr size_t MAX_DELAY = SampleRate() * MAX_DELAY_SEC;

constexpr auto DELAY_SMOOTH_MS = 500.f;

// tape params
constexpr auto WOW_FREQ = 0.4f;
constexpr auto FLUTTER_FREQ = 25.f;  // try lower
constexpr float WOW_MAX_AMP = .0092 * SampleRate();
constexpr float FLUTTER_MAX_AMP = 0.0008 * SampleRate();

constexpr float LOSS_LP_FC_MIN = 1000.f;
constexpr float LOSS_LP_FC_MAX = 6000.f;

constexpr float TAPE_DRIVE_DB = 12.f;

#endif  // CONSTANTS_HPP