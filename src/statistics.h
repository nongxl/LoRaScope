#ifndef STATISTICS_H
#define STATISTICS_H

#include "common.h"
#include <deque>
#include <map>

class StatisticsCollector {
private:
    std::map<uint32_t, FrequencyStats> freqStatsMap;
    std::deque<ScanSample> recentSamples;
    const size_t maxRecentSamples = 100;
    
    struct SlidingWindow {
        std::deque<int16_t> rssiWindow;
        size_t windowSize;
        
        SlidingWindow(size_t size = 10) : windowSize(size) {}
        
        void add(int16_t value) {
            rssiWindow.push_back(value);
            if (rssiWindow.size() > windowSize) {
                rssiWindow.pop_front();
            }
        }
        
        int16_t getAverage() {
            if (rssiWindow.empty()) return -120;
            
            int32_t sum = 0;
            for (int16_t val : rssiWindow) {
                sum += val;
            }
            return (int16_t)(sum / rssiWindow.size());
        }
        
        int16_t getMax() {
            if (rssiWindow.empty()) return -120;
            
            int16_t maxVal = rssiWindow[0];
            for (int16_t val : rssiWindow) {
                if (val > maxVal) maxVal = val;
            }
            return maxVal;
        }
        
        int16_t getMin() {
            if (rssiWindow.empty()) return -120;
            
            int16_t minVal = rssiWindow[0];
            for (int16_t val : rssiWindow) {
                if (val < minVal) minVal = val;
            }
            return minVal;
        }
        
        size_t size() {
            return rssiWindow.size();
        }
    };
    
    std::map<uint32_t, SlidingWindow> rssiWindows;
    
public:
    StatisticsCollector();
    
    void addSample(const ScanSample& sample);
    void updateStatistics();
    FrequencyStats* getStats(uint32_t frequency);
    std::vector<FrequencyStats> getAllStats();
    void cleanup(uint32_t maxAgeMs);
    
    float calculateActivityScore(const FrequencyStats& stats);
    
    void clear();
    size_t getRecentSampleCount() const;
    size_t getFrequencyCount() const;
};

#endif // STATISTICS_H
