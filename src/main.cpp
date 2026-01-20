#include <M5Cardputer.h>
#include <M5_LoRa_E220.h>
#include "common.h"
#include "lora_adapter.h"
#include "scanner.h"
#include "statistics.h"
#include "display.h"
#include "config.h"
#include "config_user.h"

LoRaAdapter* loraAdapter = nullptr;
FrequencyListener* listener = nullptr;
ScopeDisplay* display = nullptr;

volatile bool receivedSample = false;
volatile ScanSample lastSample;

void IRAM_ATTR onReceive() {
    receivedSample = true;
}

void setup() {
    USBSerial.begin(115200);
    delay(500);
    USBSerial.println("\n\n=== LoRaScope Starting ===");
    
    USBSerial.println("Step 1: Initializing M5Cardputer...");
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    M5Cardputer.Display.init();
    M5Cardputer.Display.setRotation(1);
    USBSerial.println("M5Cardputer initialized");
    
    USBSerial.println("Step 2: Creating LoRa adapter...");
    loraAdapter = LoRaAdapterFactory::createDefaultAdapter();
    USBSerial.println("LoRa adapter created");
    
    USBSerial.println("Step 3: Initializing LoRa module...");
    delay(100);
    
    bool loraInitSuccess = false;
    if (!loraAdapter || !loraAdapter->init()) {
        USBSerial.println("ERROR: Failed to initialize LoRa module!");
        loraInitSuccess = false;
    } else {
        USBSerial.println("LoRa module initialized: " + loraAdapter->getModuleName());
        loraInitSuccess = true;
    }
    
    USBSerial.println("Step 4: Creating listener...");
    listener = new FrequencyListener(loraAdapter);
    USBSerial.println("Listener created");
    
    LoRaScopeConfig scopeConfig = getUserConfig();
    USBSerial.printf("Config: %lu - %lu Hz\n", scopeConfig.startFreqHz, scopeConfig.endFreqHz);
    
    ListenerConfig config;
    config.frequencies = scopeConfig.getFrequencies();
    config.currentFreqIndex = 0;
    config.rxWindowMs = scopeConfig.rxWindowMs;
    config.bandwidth = scopeConfig.bandwidth;
    config.spreadingFactor = scopeConfig.spreadingFactor;
    config.codingRate = scopeConfig.codingRate;
    config.maxPoints = scopeConfig.maxPoints;
    
    USBSerial.printf("Generated %d frequency points\n", config.frequencies.size());
    
    USBSerial.println("Step 5: Initializing listener...");
    delay(100);
    
    bool listenerInitSuccess = false;
    if (!loraInitSuccess) {
        USBSerial.println("WARNING: Skipping listener initialization due to LoRa failure");
        listenerInitSuccess = false;
    } else if (!listener->init(config)) {
        USBSerial.println("ERROR: Failed to initialize listener!");
        listenerInitSuccess = false;
    } else {
        USBSerial.printf("Listener initialized with %d frequencies\n", config.frequencies.size());
        listenerInitSuccess = true;
    }
    
    USBSerial.println("Step 6: Creating display...");
    display = new ScopeDisplay();
    USBSerial.println("Display created");
    
    USBSerial.println("Step 7: Initializing display...");
    delay(100);
    
    if (!display->init()) {
        USBSerial.println("ERROR: Failed to initialize display!");
        while (1) {
            delay(1000);
        }
    }
    
    USBSerial.println("Display initialized");
    
    USBSerial.println("Step 8: Configuring display...");
    display->setModuleName(loraInitSuccess ? loraAdapter->getModuleName() : "No LoRa");
    display->setScanning(false);
    
    std::vector<uint32_t> freqList;
    for (const auto& freqConfig : config.frequencies) {
        freqList.push_back(freqConfig.frequency);
    }
    display->setFrequencies(freqList);
    
    if (listener) {
        display->setCurrentFreq(listener->getCurrentFrequency());
        display->setCurrentFreqIndex(listener->getCurrentFreqIndex(), config.frequencies.size());
    }
    USBSerial.println("Display configured");
    
    USBSerial.println("Step 9: Starting listener...");
    delay(100);
    
    listener->setScopeDisplay(display);
    
    USBSerial.println("Listener ready (press 's' to start)");
    
    uint32_t startFreq = config.frequencies[0].frequency;
    USBSerial.printf("Step 10: Auto-starting listener at %lu Hz...\n", startFreq);
    delay(100);
    
    if (listenerInitSuccess) {
        listener->start();
        display->setScanning(true);
        USBSerial.println("Listener auto-started");
    } else {
        display->setScanning(false);
        USBSerial.println("Listener not started due to initialization failure");
    }
    
    USBSerial.println("=== Setup Complete, Entering Loop ===");
    
    delay(500);  // 给主循环一些时间启动
}

void loop() {
    static unsigned long loopCount = 0;
    loopCount++;
    
    M5Cardputer.update();
    
    static unsigned long lastLoopDebugTime = 0;
    if (millis() - lastLoopDebugTime > 10000) {
        lastLoopDebugTime = millis();
        USBSerial.println("Loop running, count: " + String(loopCount));
    }
    
    static unsigned long lastKeyPressMillis = 0;
    const unsigned long debounceDelay = 200;
    
    if (M5Cardputer.Keyboard.isChange() && millis() - lastKeyPressMillis >= debounceDelay) {
        auto keys = M5Cardputer.Keyboard.keysState();
        
        for (auto key : keys.word) {
            switch (key) {
                case '1':
                    display->setMode(MODE_TIMELINE);
                    USBSerial.println("Mode: Timeline");
                    break;
                case '2':
                    display->setMode(MODE_HISTOGRAM);
                    USBSerial.println("Mode: Histogram");
                    break;
                case '3':
                    display->setMode(MODE_EVENTLIST);
                    USBSerial.println("Mode: Event List");
                    break;
                case '4':
                    display->setMode(MODE_STATISTICS);
                    USBSerial.println("Mode: Statistics");
                    break;
                case '5':
                    display->setMode(MODE_FREQCOMPARE);
                    USBSerial.println("Mode: Frequency Comparison");
                    break;
                case '6':
                    display->setMode(MODE_REALTIME);
                    USBSerial.println("Mode: Realtime Monitor");
                    break;
                case '0':
                    display->setMode(MODE_RADAR);
                    USBSerial.println("Mode: Radar");
                    break;
                case 's':
                    if (listener && listener->isRunning()) {
                        listener->stop();
                        display->setScanning(false);
                        USBSerial.println("Listener stopped");
                    } else if (listener) {
                        listener->start();
                        display->setScanning(true);
                        USBSerial.println("Listener started");
                    }
                    break;
                case 'c':
                    if (listener) {
                        listener->clearRadarPoints();
                        listener->clearEventStats();
                        USBSerial.println("Data cleared");
                    }
                    break;
                case '-':
                    if (listener) {
                        listener->prevFrequency();
                        uint8_t idx = listener->getCurrentFreqIndex();
                        USBSerial.printf("Previous frequency: index %d\n", idx);
                    }
                    break;
                case '=':
                    if (listener) {
                        listener->nextFrequency();
                        uint8_t idx = listener->getCurrentFreqIndex();
                        USBSerial.printf("Next frequency: index %d\n", idx);
                    }
                    break;
            }
        }
        
        lastKeyPressMillis = millis();
    }
    
    uint8_t batteryPct = M5Cardputer.Power.getBatteryLevel();
    display->setBatteryPct(batteryPct);
    
    if (listener) {
        const std::vector<RadarPoint>& points = listener->getRadarPoints();
        const EventStats& stats = listener->getEventStats();
        display->update(points, stats);
    }
    
    delay(10);
}
