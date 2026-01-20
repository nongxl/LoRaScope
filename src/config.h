#ifndef CONFIG_H
#define CONFIG_H

#include <vector>
#include "common.h"

struct LoRaScopeConfig {
    uint32_t startFreqHz;
    uint32_t endFreqHz;
    uint32_t freqStepHz;
    uint16_t rxWindowMs;
    uint8_t bandwidth;
    uint8_t spreadingFactor;
    uint8_t codingRate;
    uint16_t maxPoints;
    
    LoRaScopeConfig()
        : startFreqHz(410125000)
        , endFreqHz(493125000)
        , freqStepHz(1000000)
        , rxWindowMs(1000)
        , bandwidth(125)
        , spreadingFactor(7)
        , codingRate(5)
        , maxPoints(100) {}
    
    std::vector<FrequencyConfig> getFrequencies() const {
        std::vector<FrequencyConfig> freqs;
        
        uint32_t freq = startFreqHz;
        while (freq <= endFreqHz) {
            freqs.push_back(FrequencyConfig(freq, rxWindowMs, bandwidth, spreadingFactor, codingRate));
            freq += freqStepHz;
        }
        
        return freqs;
    }
};

#endif
