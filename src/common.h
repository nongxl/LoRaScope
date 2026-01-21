#ifndef COMMON_H
#define COMMON_H

#include <Arduino.h>
#include <vector>

// LoRa 模块类型枚举
enum LoRaModuleType {
    LORA_E220_433,    // E220-433T30D
    LORA_E220_868,    // E220-868T30D
    LORA_E220_915,    // E220-915T30D
    LORA_SX1262,      // SX1262 模块
    LORA_RF95,        // RF95 模块
    LORA_CUSTOM       // 自定义模块
};

// 频点配置
struct FrequencyConfig {
    uint32_t frequency;       // 频率 (Hz)
    uint16_t dwellTime;        // 驻留时间 (ms)
    uint16_t bandwidth;        // 带宽 (125/250/500 kHz)
    uint8_t spreadingFactor;   // 扩频因子 (7-12)
    uint8_t codingRate;        // 编码率 (4/5 to 4/8)
    
    FrequencyConfig(uint32_t freq = 433000000, uint32_t dwell = 1000, 
                    uint16_t bw = 125, uint8_t sf = 7, uint8_t cr = 5)
        : frequency(freq), dwellTime(dwell), bandwidth(bw), 
          spreadingFactor(sf), codingRate(cr) {}
    
    String toString() const {
        return "Freq: " + String(frequency / 1000000.0, 3) + " MHz, " +
               "Dwell: " + String(dwellTime) + " ms, " +
               "BW: " + String(bandwidth) + " kHz, " +
               "SF: " + String(spreadingFactor) + ", " +
               "CR: 4/" + String(codingRate);
    }
};

// 扫描样本
struct ScanSample {
    uint32_t frequency;
    int16_t rssi;            // 信号强度 (dBm)
    int16_t snr;             // 信噪比 (dB)
    bool packetReceived;     // 是否接收到有效数据包
    uint32_t timestamp;
    uint8_t errorCount;      // CRC错误计数
    
    ScanSample() 
        : frequency(0), rssi(-120), snr(-20), packetReceived(false), 
          timestamp(0), errorCount(0) {}
};

// 频点统计信息
struct FrequencyStats {
    uint32_t frequency;
    uint16_t sampleCount;     // 采样次数
    int16_t avgRssi;         // 平均 RSSI
    int16_t maxRssi;         // 最大 RSSI
    int16_t minRssi;         // 最小 RSSI
    uint16_t packetCount;    // 接收到的有效包数
    float activityScore;     // 活动评分 (0.0-1.0)
    uint32_t lastSeen;       // 最后检测到活动的时间
    
    FrequencyStats() 
        : frequency(0), sampleCount(0), avgRssi(-120), maxRssi(-120), 
          minRssi(-120), packetCount(0), activityScore(0.0), lastSeen(0) {}
};

// 扫描配置
struct ScannerConfig {
    std::vector<FrequencyConfig> frequencies;
    uint32_t scanInterval;   // 扫描间隔 (ms)
    uint8_t scanMode;        // 0=循环扫描, 1=随机扫描
    bool continuousScan;     // 是否连续扫描
    uint8_t samplesPerFreq;  // 每个频点采样次数
    
    ScannerConfig() 
        : scanInterval(100), scanMode(0), continuousScan(true), samplesPerFreq(3) {}
};

// 显示模式
enum DisplayMode {
    MODE_TIMELINE,     // 时间线视图（事件驱动）
    MODE_HISTOGRAM,    // 直方图视图
    MODE_EVENTLIST,    // 事件列表视图
    MODE_STATISTICS,    // 统计视图
    MODE_FREQCOMPARE,   // 频点对比视图
    MODE_REALTIME,      // 实时监测视图
    MODE_RADAR          // 雷达视图
};

// 事件类型
enum EventType {
    EVENT_RX_DONE,        // 成功接收
    EVENT_RX_CRC_ERROR,   // CRC 错误
    EVENT_RX_TIMEOUT      // 超时（不记录为雷达点）
};

// 雷达点（时频接收事件）
struct RadarPoint {
    uint32_t timestamp;      // 时间戳 (ms)
    uint32_t frequency;      // 频率 (Hz)
    int16_t rssi;            // 信号强度 (dBm)
    int16_t snr;             // 信噪比 (dB)
    uint8_t packetLength;    // 数据包长度
    EventType eventType;     // 事件类型
    
    RadarPoint() 
        : timestamp(0), frequency(0), rssi(-120), snr(-20), 
          packetLength(0), eventType(EVENT_RX_TIMEOUT) {}
};

// 事件统计信息
struct EventStats {
    uint32_t totalEvents;      // 总事件数
    uint32_t rxDoneCount;      // RX_DONE 次数
    uint32_t rxErrorCount;     // RX_ERROR 次数
    int16_t avgRssi;           // 平均 RSSI
    int16_t maxRssi;           // 最大 RSSI
    int16_t minRssi;           // 最小 RSSI
    uint32_t lastEventTime;    // 最后一次事件时间
    uint32_t firstEventTime;   // 第一次事件时间
    
    EventStats() 
        : totalEvents(0), rxDoneCount(0), rxErrorCount(0), 
          avgRssi(-120), maxRssi(-120), minRssi(-120), 
          lastEventTime(0), firstEventTime(0) {}
};

// 监听配置
struct ListenerConfig {
    std::vector<FrequencyConfig> frequencies;
    uint16_t currentFreqIndex;
    uint16_t rxWindowMs;
    uint16_t bandwidth;
    uint8_t spreadingFactor;
    uint8_t codingRate;
    uint16_t maxPoints;
    
    ListenerConfig() 
        : currentFreqIndex(0), rxWindowMs(1000), bandwidth(125), 
          spreadingFactor(7), codingRate(5), maxPoints(100) {}
};

// 颜色定义
#define BG_COLOR TFT_BLACK
#define UX_COLOR_DARK 0x1082
#define UX_COLOR_LIGHT 0x3186
#define UX_COLOR_ACCENT 0xE73C
#define UX_COLOR_ACCENT2 0xFD20
#define COLOR_ORANGE 0xF800
#define COLOR_LIGHTGRAY 0x632C
#define COLOR_SILVER 0xC618

// 工具函数
inline float normalize(float value, float minVal, float maxVal) {
    if (maxVal <= minVal) return 0.0;
    return constrain((value - minVal) / (maxVal - minVal), 0.0, 1.0);
}

inline float constrain_float(float value, float minVal, float maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}

#endif // COMMON_H
