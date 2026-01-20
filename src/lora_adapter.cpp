#include "lora_adapter.h"
#include <M5_LoRa_E220.h>
#include <M5Cardputer.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// E220 适配器实现
E220Adapter::E220Adapter(LoRa_E220* loraModule, LoRaModuleType type)
    : lora(loraModule), moduleType(type), initialized(false) {
}

bool E220Adapter::init() {
    if (initialized) return true;
    
    USBSerial.println("[E220] Initializing LoRa_E220...");
    
    USBSerial.println("[E220] Calling Init()...");
    lora->Init(&Serial2, 9600, SERIAL_8N1, 1, 2);
    _serial = &Serial2;
    USBSerial.println("[E220] Init() completed");
    
    USBSerial.println("[E220] Setting default config...");
    LoRaConfigItem_t config;
    lora->SetDefaultConfigValue(config);
    USBSerial.println("[E220] Default config set");
    
    USBSerial.println("[E220] Calling InitLoRaSetting()...");
    int result = lora->InitLoRaSetting(config);
    USBSerial.println("[E220] InitLoRaSetting() returned: " + String(result));
    
    initialized = true;
    USBSerial.println("[E220] Initialization complete");
    return true;
}

bool E220Adapter::setFrequency(uint32_t freqHz) {
    if (!initialized) return false;
    
    USBSerial.println("[E220] Setting frequency: " + String(freqHz) + " Hz");
    
    LoRaConfigItem_t config;
    lora->SetDefaultConfigValue(config);
    
    uint16_t channel = 0;
    if (moduleType == LORA_E220_433) {
        channel = (freqHz - 410125000) / 1000000;
        USBSerial.println("[E220] 433MHz mode, calculated channel: " + String(channel));
    } else if (moduleType == LORA_E220_868) {
        channel = (freqHz - 850000000) / 1000000;
        USBSerial.println("[E220] 868MHz mode, calculated channel: " + String(channel));
    } else if (moduleType == LORA_E220_915) {
        channel = (freqHz - 902000000) / 1000000;
        USBSerial.println("[E220] 915MHz mode, calculated channel: " + String(channel));
    }
    
    config.own_channel = channel;
    USBSerial.println("[E220] Setting own_channel to: " + String(config.own_channel));
    
    int result = lora->InitLoRaSetting(config);
    USBSerial.println("[E220] InitLoRaSetting returned: " + String(result));
    
    return result == 0;
}

bool E220Adapter::setBandwidth(uint8_t bandwidth) {
    if (!initialized) return false;
    
    LoRaConfigItem_t config;
    lora->SetDefaultConfigValue(config);
    
    switch (bandwidth) {
        case 125:
            config.air_data_rate = DATA_RATE_2_4Kbps;
            break;
        case 250:
            config.air_data_rate = DATA_RATE_9_6Kbps;
            break;
        case 500:
            config.air_data_rate = DATA_RATE_19_2Kbps;
            break;
        default:
            config.air_data_rate = DATA_RATE_2_4Kbps;
    }
    
    return lora->InitLoRaSetting(config) == 0;
}

bool E220Adapter::setSpreadingFactor(uint8_t sf) {
    if (!initialized) return false;
    
    LoRaConfigItem_t config;
    lora->SetDefaultConfigValue(config);
    
    if (sf >= 7 && sf <= 12) {
        uint8_t sfIndex = sf - 7;
        if (sfIndex <= 7) {
            config.air_data_rate = sfIndex;
        }
    }
    
    return lora->InitLoRaSetting(config) == 0;
}

bool E220Adapter::setCodingRate(uint8_t cr) {
    if (!initialized) return false;
    
    LoRaConfigItem_t config;
    lora->SetDefaultConfigValue(config);
    
    config.rssi_ambient_noise_flag = (cr == 5) ? RSSI_AMBIENT_NOISE_ENABLE : RSSI_AMBIENT_NOISE_DISABLE;
    
    return lora->InitLoRaSetting(config) == 0;
}

int16_t E220Adapter::getRSSI() {
    if (!initialized) return -120;
    
    USBSerial.println("[E220] Checking for available data...");
    
    if (_serial->available() > 0) {
        USBSerial.printf("[E220] Data available: %d bytes\n", _serial->available());
        
        RecvFrame_t frame;
        USBSerial.println("[E220] Calling RecieveFrame()...");
        int result = lora->RecieveFrame(&frame);
        USBSerial.printf("[E220] RecieveFrame() returned: %d\n", result);
        
        if (result == 0) {
            USBSerial.printf("[E220] RSSI: %d dBm\n", frame.rssi);
            return frame.rssi;
        }
    } else {
        USBSerial.println("[E220] No data available");
    }
    
    USBSerial.println("[E220] Returning -120 dBm");
    return -120;
}

int16_t E220Adapter::getSNR() {
    if (!initialized) return -20;
    
    if (_serial->available() > 0) {
        RecvFrame_t frame;
        int result = lora->RecieveFrame(&frame);
        
        if (result == 0 && frame.recv_data_len > 0) {
            return 10;
        }
    }
    
    return -20;
}

bool E220Adapter::receivePacket(uint8_t* buffer, size_t* length) {
    if (!initialized || !buffer || !length) return false;
    
    if (_serial->available() > 0) {
        RecvFrame_t frame;
        int result = lora->RecieveFrame(&frame);
        
        if (result == 0 && frame.recv_data_len > 0) {
            *length = frame.recv_data_len;
            memcpy(buffer, frame.recv_data, *length);
            return true;
        }
    }
    
    return false;
}

void E220Adapter::standby() {
    if (initialized) {
        LoRaConfigItem_t config;
        lora->SetDefaultConfigValue(config);
        lora->InitLoRaSetting(config);
    }
}

bool E220Adapter::sleep() {
    if (initialized) {
        LoRaConfigItem_t config;
        lora->SetDefaultConfigValue(config);
        config.transmission_method_type = UART_TT_MODE;
        lora->InitLoRaSetting(config);
        return true;
    }
    return false;
}

LoRaModuleType E220Adapter::getModuleType() {
    return moduleType;
}

String E220Adapter::getModuleName() {
    switch (moduleType) {
        case LORA_E220_433:
            return "E220-433";
        case LORA_E220_868:
            return "E220-868";
        case LORA_E220_915:
            return "E220-915";
        default:
            return "E220";
    }
}

#ifdef LORA_MODULE
// SX1262 适配器实现
SX1262Adapter::SX1262Adapter(SX1262* loraModule, SPIClass* spiClass, 
                              uint8_t cs, uint8_t irq, uint8_t rst, uint8_t busy)
    : lora(loraModule), spi(spiClass), csPin(cs), irqPin(irq), 
      rstPin(rst), busyPin(busy), initialized(false) {
}

bool SX1262Adapter::init() {
    if (initialized) return true;
    
    int state = lora->begin();
    if (state != RADIOLIB_ERR_NONE) {
        return false;
    }
    
    initialized = true;
    return true;
}

bool SX1262Adapter::setFrequency(uint32_t freqHz) {
    if (!initialized) return false;
    
    int state = lora->setFrequency(freqHz);
    return state == RADIOLIB_ERR_NONE;
}

bool SX1262Adapter::setBandwidth(uint8_t bandwidth) {
    if (!initialized) return false;
    
    float bwKhz = bandwidth;
    int state = lora->setBandwidth(bwKhz);
    return state == RADIOLIB_ERR_NONE;
}

bool SX1262Adapter::setSpreadingFactor(uint8_t sf) {
    if (!initialized) return false;
    
    int state = lora->setSpreadingFactor(sf);
    return state == RADIOLIB_ERR_NONE;
}

bool SX1262Adapter::setCodingRate(uint8_t cr) {
    if (!initialized) return false;
    
    uint8_t crValue = (cr - 4);
    int state = lora->setCodingRate(crValue);
    return state == RADIOLIB_ERR_NONE;
}

int16_t SX1262Adapter::getRSSI() {
    if (!initialized) return -120;
    
    return lora->getRSSI();
}

int16_t SX1262Adapter::getSNR() {
    if (!initialized) return -20;
    
    return lora->getSNR();
}

bool SX1262Adapter::receivePacket(uint8_t* buffer, size_t* length) {
    if (!initialized || !buffer || !length) return false;
    
    int state = lora->receive(buffer, *length);
    if (state == RADIOLIB_ERR_NONE) {
        *length = lora->getPacketLength();
        return true;
    }
    
    return false;
}

void SX1262Adapter::standby() {
    if (initialized) {
        lora->standby();
    }
}

bool SX1262Adapter::sleep() {
    if (initialized) {
        int state = lora->sleep();
        return state == RADIOLIB_ERR_NONE;
    }
    return false;
}

LoRaModuleType SX1262Adapter::getModuleType() {
    return LORA_SX1262;
}

String SX1262Adapter::getModuleName() {
    return "SX1262";
}

// RF95 适配器实现
RF95Adapter::RF95Adapter(RFM95* loraModule, SPIClass* spiClass, 
                          uint8_t cs, uint8_t irq, uint8_t rst)
    : lora(loraModule), spi(spiClass), csPin(cs), irqPin(irq), 
      rstPin(rst), initialized(false) {
}

bool RF95Adapter::init() {
    if (initialized) return true;
    
    int state = lora->begin();
    if (state != RADIOLIB_ERR_NONE) {
        return false;
    }
    
    initialized = true;
    return true;
}

bool RF95Adapter::setFrequency(uint32_t freqHz) {
    if (!initialized) return false;
    
    int state = lora->setFrequency(freqHz);
    return state == RADIOLIB_ERR_NONE;
}

bool RF95Adapter::setBandwidth(uint8_t bandwidth) {
    if (!initialized) return false;
    
    float bwKhz = bandwidth;
    int state = lora->setBandwidth(bwKhz);
    return state == RADIOLIB_ERR_NONE;
}

bool RF95Adapter::setSpreadingFactor(uint8_t sf) {
    if (!initialized) return false;
    
    int state = lora->setSpreadingFactor(sf);
    return state == RADIOLIB_ERR_NONE;
}

bool RF95Adapter::setCodingRate(uint8_t cr) {
    if (!initialized) return false;
    
    uint8_t crValue = (cr - 4);
    int state = lora->setCodingRate(crValue);
    return state == RADIOLIB_ERR_NONE;
}

int16_t RF95Adapter::getRSSI() {
    if (!initialized) return -120;
    
    return lora->getRSSI();
}

int16_t RF95Adapter::getSNR() {
    if (!initialized) return -20;
    
    return lora->getSNR();
}

bool RF95Adapter::receivePacket(uint8_t* buffer, size_t* length) {
    if (!initialized || !buffer || !length) return false;
    
    int state = lora->receive(buffer, *length);
    if (state == RADIOLIB_ERR_NONE) {
        *length = lora->getPacketLength();
        return true;
    }
    
    return false;
}

void RF95Adapter::standby() {
    if (initialized) {
        lora->standby();
    }
}

bool RF95Adapter::sleep() {
    if (initialized) {
        int state = lora->sleep();
        return state == RADIOLIB_ERR_NONE;
    }
    return false;
}

LoRaModuleType RF95Adapter::getModuleType() {
    return LORA_RF95;
}

String RF95Adapter::getModuleName() {
    return "RF95";
}
#endif

// LoRa 模块工厂实现
LoRaAdapter* LoRaAdapterFactory::createAdapter(LoRaModuleType type, void* config) {
    switch (type) {
        case LORA_E220_433:
        case LORA_E220_868:
        case LORA_E220_915: {
            LoRa_E220* lora = static_cast<LoRa_E220*>(config);
            return new E220Adapter(lora, type);
        }
#ifdef LORA_MODULE
        case LORA_SX1262: {
            struct SX1262Config {
                SX1262* lora;
                SPIClass* spi;
                uint8_t cs, irq, rst, busy;
            };
            SX1262Config* cfg = static_cast<SX1262Config*>(config);
            return new SX1262Adapter(cfg->lora, cfg->spi, cfg->cs, cfg->irq, cfg->rst, cfg->busy);
        }
        case LORA_RF95: {
            struct RF95Config {
                RFM95* lora;
                SPIClass* spi;
                uint8_t cs, irq, rst;
            };
            RF95Config* cfg = static_cast<RF95Config*>(config);
            return new RF95Adapter(cfg->lora, cfg->spi, cfg->cs, cfg->irq, cfg->rst);
        }
#endif
        default:
            return nullptr;
    }
}

LoRaAdapter* LoRaAdapterFactory::createDefaultAdapter() {
    static LoRa_E220 e220;
    return new E220Adapter(&e220, LORA_E220_433);
}
