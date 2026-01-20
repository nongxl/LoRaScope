#ifndef SCANNER_H
#define SCANNER_H

#include "common.h"
#include "lora_adapter.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class ScopeDisplay;

class FrequencyListener {
private:
    LoRaAdapter* lora;
    ListenerConfig config;
    TaskHandle_t listenTaskHandle;
    volatile bool isListening;
    volatile bool shouldStop;
    
    uint32_t lastEventTime;
    std::vector<RadarPoint> radarPoints;
    EventStats eventStats;
    uint8_t packetBuffer[256];
    
    void listenTaskWrapper(void* pvParameters);
    void listenTask();
    bool setFrequency(uint32_t freq);
    void handleRxDone(const RecvFrame_t& frame);
    void handleRxError();
    
    ScopeDisplay* scopeDisplay;
    
public:
    FrequencyListener(LoRaAdapter* loraModule);
    ~FrequencyListener();
    
    bool init(const ListenerConfig& cfg);
    void start();
    void stop();
    
    void setScopeDisplay(ScopeDisplay* disp);
    
    bool isRunning() const;
    uint32_t getCurrentFrequency() const;
    uint8_t getCurrentFreqIndex() const;
    void nextFrequency();
    void prevFrequency();
    ListenerConfig getConfig() const;
    void setConfig(const ListenerConfig& cfg);
    
    const std::vector<RadarPoint>& getRadarPoints() const;
    const EventStats& getEventStats() const;
    void clearRadarPoints();
    void clearEventStats();
};

#endif // SCANNER_H
