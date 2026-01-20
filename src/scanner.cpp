#include "scanner.h"
#include "display.h"
#include "statistics.h"
#include <M5Cardputer.h>

FrequencyListener::FrequencyListener(LoRaAdapter* loraModule)
    : lora(loraModule), listenTaskHandle(nullptr),
      isListening(false), shouldStop(false), lastEventTime(0), scopeDisplay(nullptr) {
}

FrequencyListener::~FrequencyListener() {
    stop();
}

bool FrequencyListener::init(const ListenerConfig& cfg) {
    config = cfg;
    
    if (!lora || !lora->init()) {
        USBSerial.println("[Listener] Failed to initialize LoRa adapter");
        return false;
    }
    
    if (config.frequencies.empty()) {
        USBSerial.println("[Listener] No frequencies configured");
        return false;
    }
    
    if (!setFrequency(config.frequencies[config.currentFreqIndex].frequency)) {
        USBSerial.println("[Listener] Failed to set initial frequency");
        return false;
    }
    
    if (!lora->setBandwidth(config.bandwidth)) {
        USBSerial.println("[Listener] Failed to set bandwidth");
    }
    
    delay(20);
    
    if (!lora->setSpreadingFactor(config.spreadingFactor)) {
        USBSerial.println("[Listener] Failed to set spreading factor");
    }
    
    delay(20);
    
    if (!lora->setCodingRate(config.codingRate)) {
        USBSerial.println("[Listener] Failed to set coding rate");
    }
    
    delay(50);
    
    USBSerial.printf("[Listener] Initialized with %d frequencies, RX window: %u ms\n", 
        config.frequencies.size(), config.rxWindowMs);
    
    return true;
}

void FrequencyListener::start() {
    if (isListening) return;
    
    isListening = true;
    shouldStop = false;
    
    USBSerial.println("[Listener] Starting...");
    
    xTaskCreate(
        [](void* pvParameters) {
            FrequencyListener* listener = static_cast<FrequencyListener*>(pvParameters);
            listener->listenTaskWrapper(pvParameters);
        },
        "ListenTask",
        4096,
        this,
        1,
        &listenTaskHandle
    );
}

void FrequencyListener::stop() {
    if (!isListening) return;
    
    USBSerial.println("[Listener] Stopping...");
    shouldStop = true;
    
    if (listenTaskHandle) {
        vTaskDelay(pdMS_TO_TICKS(100));
        if (isListening) {
            vTaskDelete(listenTaskHandle);
        }
        listenTaskHandle = nullptr;
    }
    
    isListening = false;
}

void FrequencyListener::listenTaskWrapper(void* pvParameters) {
    listenTask();
    vTaskDelete(nullptr);
}

void FrequencyListener::listenTask() {
    USBSerial.println("[Listener] Task started");
    
    while (!shouldStop) {
        uint32_t rxStartTime = millis();
        bool eventReceived = false;
        
        USBSerial.printf("[Listener] RX window started at %lu ms\n", rxStartTime);
        
        while (millis() - rxStartTime < config.rxWindowMs && !shouldStop) {
            if (Serial2.available() > 0) {
                USBSerial.println("[Listener] Data available on Serial2");
                
                RecvFrame_t frame;
                int result = lora->receiveFrame(&frame);
                
                USBSerial.printf("[Listener] RecieveFrame returned: %d\n", result);
                
                if (result == 0) {
                    handleRxDone(frame);
                    eventReceived = true;
                    break;
                } else if (result == 1) {
                    handleRxError();
                    eventReceived = true;
                    break;
                }
            }
            
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        
        if (!eventReceived) {
            USBSerial.println("[Listener] RX timeout (no event)");
        }
        
        if (!shouldStop) {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
    
    USBSerial.println("[Listener] Task stopped");
    isListening = false;
}

bool FrequencyListener::setFrequency(uint32_t freq) {
    if (!lora) return false;
    
    USBSerial.printf("[Listener] Setting frequency: %lu Hz\n", freq);
    
    if (!lora->setFrequency(freq)) {
        USBSerial.println("[Listener] Failed to set frequency");
        return false;
    }
    
    delay(100);
    
    USBSerial.println("[Listener] Frequency set successfully");
    return true;
}

void FrequencyListener::nextFrequency() {
    if (config.frequencies.empty()) return;
    
    config.currentFreqIndex = (config.currentFreqIndex + 1) % config.frequencies.size();
    
    uint32_t newFreq = config.frequencies[config.currentFreqIndex].frequency;
    USBSerial.printf("[Listener] Switching to next frequency: %lu Hz (index: %d)\n", 
        newFreq, config.currentFreqIndex);
    
    setFrequency(newFreq);
    
    if (scopeDisplay) {
        scopeDisplay->setCurrentFreq(newFreq);
        scopeDisplay->setCurrentFreqIndex(config.currentFreqIndex, config.frequencies.size());
    }
}

void FrequencyListener::prevFrequency() {
    if (config.frequencies.empty()) return;
    
    config.currentFreqIndex = (config.currentFreqIndex == 0) 
        ? config.frequencies.size() - 1 
        : config.currentFreqIndex - 1;
    
    uint32_t newFreq = config.frequencies[config.currentFreqIndex].frequency;
    USBSerial.printf("[Listener] Switching to previous frequency: %lu Hz (index: %d)\n", 
        newFreq, config.currentFreqIndex);
    
    setFrequency(newFreq);
    
    if (scopeDisplay) {
        scopeDisplay->setCurrentFreq(newFreq);
        scopeDisplay->setCurrentFreqIndex(config.currentFreqIndex, config.frequencies.size());
    }
}

void FrequencyListener::handleRxDone(const RecvFrame_t& frame) {
    int16_t rssi = frame.rssi;
    
    if (rssi < -120 || rssi > -50) {
        USBSerial.printf("[Listener] Invalid RSSI: %d dBm, ignoring\n", rssi);
        return;
    }
    
    RadarPoint point;
    point.timestamp = millis();
    point.frequency = config.frequencies[config.currentFreqIndex].frequency;
    point.rssi = rssi;
    point.snr = -20;
    point.packetLength = frame.recv_data_len;
    point.eventType = EVENT_RX_DONE;
    
    USBSerial.printf("[Listener] RX_DONE - Time: %lu, RSSI: %d dBm, Len: %d\n",
        point.timestamp, point.rssi, point.packetLength);
    
    radarPoints.push_back(point);
    
    if (radarPoints.size() > config.maxPoints) {
        radarPoints.erase(radarPoints.begin());
    }
    
    eventStats.totalEvents++;
    eventStats.rxDoneCount++;
    eventStats.lastEventTime = point.timestamp;
    
    if (eventStats.firstEventTime == 0) {
        eventStats.firstEventTime = point.timestamp;
    }
    
    if (point.rssi > eventStats.maxRssi) {
        eventStats.maxRssi = point.rssi;
    }
    
    if (point.rssi < eventStats.minRssi || eventStats.minRssi == -120) {
        eventStats.minRssi = point.rssi;
    }
    
    eventStats.avgRssi = (eventStats.avgRssi * (eventStats.totalEvents - 1) + point.rssi) / eventStats.totalEvents;
    
    if (scopeDisplay) {
        scopeDisplay->setCurrentFreq(point.frequency);
        scopeDisplay->setCurrentRssi(point.rssi);
    }
}

void FrequencyListener::handleRxError() {
    RadarPoint point;
    point.timestamp = millis();
    point.frequency = config.frequencies[config.currentFreqIndex].frequency;
    point.rssi = -120;
    point.snr = -20;
    point.packetLength = 0;
    point.eventType = EVENT_RX_CRC_ERROR;
    
    USBSerial.printf("[Listener] RX_CRC_ERROR - Time: %lu\n", point.timestamp);
    
    radarPoints.push_back(point);
    
    if (radarPoints.size() > config.maxPoints) {
        radarPoints.erase(radarPoints.begin());
    }
    
    eventStats.totalEvents++;
    eventStats.rxErrorCount++;
    eventStats.lastEventTime = point.timestamp;
    
    if (eventStats.firstEventTime == 0) {
        eventStats.firstEventTime = point.timestamp;
    }
}

void FrequencyListener::setScopeDisplay(ScopeDisplay* disp) {
    scopeDisplay = disp;
}

bool FrequencyListener::isRunning() const {
    return isListening;
}

uint32_t FrequencyListener::getCurrentFrequency() const {
    if (config.frequencies.empty()) return 0;
    return config.frequencies[config.currentFreqIndex].frequency;
}

uint8_t FrequencyListener::getCurrentFreqIndex() const {
    return config.currentFreqIndex;
}

ListenerConfig FrequencyListener::getConfig() const {
    return config;
}

void FrequencyListener::setConfig(const ListenerConfig& cfg) {
    config = cfg;
}

const std::vector<RadarPoint>& FrequencyListener::getRadarPoints() const {
    return radarPoints;
}

const EventStats& FrequencyListener::getEventStats() const {
    return eventStats;
}

void FrequencyListener::clearRadarPoints() {
    radarPoints.clear();
    USBSerial.println("[Listener] Radar points cleared");
}

void FrequencyListener::clearEventStats() {
    eventStats = EventStats();
    USBSerial.println("[Listener] Event stats cleared");
}
