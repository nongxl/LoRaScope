#ifndef LORA_ADAPTER_H
#define LORA_ADAPTER_H

#include "common.h"
#include <M5_LoRa_E220.h>

#ifdef LORA_MODULE
#include <RadioLib.h>
#endif

// LoRa 模块抽象接口
class LoRaAdapter {
public:
    virtual ~LoRaAdapter() {}
    
    virtual bool init() = 0;
    virtual bool setFrequency(uint32_t freqHz) = 0;
    virtual bool setBandwidth(uint16_t bandwidth) = 0;
    virtual bool setSpreadingFactor(uint8_t sf) = 0;
    virtual bool setCodingRate(uint8_t cr) = 0;
    virtual int16_t getRSSI() = 0;
    virtual int16_t getSNR() = 0;
    virtual bool receivePacket(uint8_t* buffer, size_t* length) = 0;
    virtual void standby() = 0;
    virtual bool sleep() = 0;
    virtual LoRaModuleType getModuleType() = 0;
    virtual String getModuleName() = 0;
    
    virtual int receiveFrame(void* frame) = 0;
};

// E220 模块适配器
class E220Adapter : public LoRaAdapter {
private:
    LoRa_E220* lora;
    HardwareSerial* _serial;
    LoRaModuleType moduleType;
    bool initialized;
    
public:
    E220Adapter(LoRa_E220* loraModule, LoRaModuleType type = LORA_E220_433);
    
    bool init() override;
    bool setFrequency(uint32_t freqHz) override;
    bool setBandwidth(uint16_t bandwidth) override;
    bool setSpreadingFactor(uint8_t sf) override;
    bool setCodingRate(uint8_t cr) override;
    int16_t getRSSI() override;
    int16_t getSNR() override;
    bool receivePacket(uint8_t* buffer, size_t* length) override;
    void standby() override;
    bool sleep() override;
    LoRaModuleType getModuleType() override;
    String getModuleName() override;
    
    int receiveFrame(void* frame) override {
        return lora->RecieveFrame((RecvFrame_t*)frame);
    }
    
    LoRa_E220* getLoRaModule() { return lora; }
};

#ifdef LORA_MODULE
// SX1262 模块适配器 (使用 RadioLib)
class SX1262Adapter : public LoRaAdapter {
private:
    SX1262* lora;
    SPIClass* spi;
    uint8_t csPin;
    uint8_t irqPin;
    uint8_t rstPin;
    uint8_t busyPin;
    bool initialized;
    
public:
    SX1262Adapter(SX1262* loraModule, SPIClass* spiClass, 
                  uint8_t cs, uint8_t irq, uint8_t rst, uint8_t busy);
    
    bool init() override;
    bool setFrequency(uint32_t freqHz) override;
    bool setBandwidth(uint16_t bandwidth) override;
    bool setSpreadingFactor(uint8_t sf) override;
    bool setCodingRate(uint8_t cr) override;
    int16_t getRSSI() override;
    int16_t getSNR() override;
    bool receivePacket(uint8_t* buffer, size_t* length) override;
    void standby() override;
    bool sleep() override;
    LoRaModuleType getModuleType() override;
    String getModuleName() override;
    
    int receiveFrame(void* frame) override {
        return -1;
    }
};

// RF95 模块适配器 (使用 RadioLib)
class RF95Adapter : public LoRaAdapter {
private:
    RFM95* lora;
    SPIClass* spi;
    uint8_t csPin;
    uint8_t irqPin;
    uint8_t rstPin;
    bool initialized;
    
public:
    RF95Adapter(RFM95* loraModule, SPIClass* spiClass, 
                uint8_t cs, uint8_t irq, uint8_t rst);
    
    bool init() override;
    bool setFrequency(uint32_t freqHz) override;
    bool setBandwidth(uint16_t bandwidth) override;
    bool setSpreadingFactor(uint8_t sf) override;
    bool setCodingRate(uint8_t cr) override;
    int16_t getRSSI() override;
    int16_t getSNR() override;
    bool receivePacket(uint8_t* buffer, size_t* length) override;
    void standby() override;
    bool sleep() override;
    LoRaModuleType getModuleType() override;
    String getModuleName() override;
    
    int receiveFrame(void* frame) override {
        return -1;
    }
};

#endif // LORA_MODULE

// LoRa 模块工厂
class LoRaAdapterFactory {
public:
    static LoRaAdapter* createAdapter(LoRaModuleType type, void* config);
    static LoRaAdapter* createDefaultAdapter();
};

#endif // LORA_ADAPTER_H
