#ifndef CONFIG_USER_H
#define CONFIG_USER_H

#include "config.h"

LoRaScopeConfig getUserConfig() {
    LoRaScopeConfig config;
    
    config.startFreqHz = 410125000;
    config.endFreqHz = 493125000;
    config.freqStepHz = 100000;
    config.rxWindowMs = 1000;
    config.bandwidth = 125;
    config.spreadingFactor = 9;
    config.codingRate = 5;
    config.maxPoints = 100;
    
    return config;
}

#endif
