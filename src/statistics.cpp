#include "statistics.h"
#include <algorithm>
#include <cmath>

StatisticsCollector::StatisticsCollector() {
}

void StatisticsCollector::addSample(const ScanSample& sample) {
    recentSamples.push_back(sample);
    
    if (recentSamples.size() > maxRecentSamples) {
        recentSamples.pop_front();
    }
    
    FrequencyStats& stats = freqStatsMap[sample.frequency];
    
    if (stats.frequency == 0) {
        stats.frequency = sample.frequency;
    }
    
    stats.sampleCount++;
    stats.lastSeen = sample.timestamp;
    
    if (sample.packetReceived) {
        stats.packetCount++;
    }
    
    rssiWindows[sample.frequency].add(sample.rssi);
}

void StatisticsCollector::updateStatistics() {
    for (auto& pair : freqStatsMap) {
        FrequencyStats& stats = pair.second;
        SlidingWindow& window = rssiWindows[pair.first];
        
        if (window.size() > 0) {
            stats.avgRssi = window.getAverage();
            stats.maxRssi = window.getMax();
            stats.minRssi = window.getMin();
        }
        
        stats.activityScore = calculateActivityScore(stats);
    }
}

FrequencyStats* StatisticsCollector::getStats(uint32_t frequency) {
    auto it = freqStatsMap.find(frequency);
    if (it != freqStatsMap.end()) {
        return &(it->second);
    }
    return nullptr;
}

std::vector<FrequencyStats> StatisticsCollector::getAllStats() {
    std::vector<FrequencyStats> result;
    result.reserve(freqStatsMap.size());
    
    for (auto& pair : freqStatsMap) {
        result.push_back(pair.second);
    }
    
    std::sort(result.begin(), result.end(), 
        [](const FrequencyStats& a, const FrequencyStats& b) {
            return a.activityScore > b.activityScore;
        });
    
    return result;
}

void StatisticsCollector::cleanup(uint32_t maxAgeMs) {
    uint32_t currentTime = millis();
    
    auto it = freqStatsMap.begin();
    while (it != freqStatsMap.end()) {
        if (currentTime - it->second.lastSeen > maxAgeMs) {
            rssiWindows.erase(it->first);
            it = freqStatsMap.erase(it);
        } else {
            ++it;
        }
    }
    
    auto sampleIt = recentSamples.begin();
    while (sampleIt != recentSamples.end()) {
        if (currentTime - sampleIt->timestamp > maxAgeMs) {
            sampleIt = recentSamples.erase(sampleIt);
        } else {
            ++sampleIt;
        }
    }
}

float StatisticsCollector::calculateActivityScore(const FrequencyStats& stats) {
    if (stats.sampleCount == 0) return 0.0;
    
    float rssiScore = 0.0;
    float packetScore = 0.0;
    float stabilityScore = 0.0;
    float freshnessScore = 0.0;
    
    rssiScore = normalize(stats.avgRssi, -120.0f, -50.0f);
    
    packetScore = std::min(1.0f, (float)stats.packetCount / (float)stats.sampleCount);
    
    float rssiRange = stats.maxRssi - stats.minRssi;
    stabilityScore = 1.0f - normalize(rssiRange, 0.0f, 30.0f);
    
    uint32_t age = millis() - stats.lastSeen;
    float maxAge = 5.0f * 60.0f * 1000.0f;
    freshnessScore = std::max(0.0f, 1.0f - (float)age / maxAge);
    
    float score = 0.35f * rssiScore + 
                  0.30f * packetScore + 
                  0.20f * stabilityScore + 
                  0.15f * freshnessScore;
    
    return constrain_float(score, 0.0f, 1.0f);
}

void StatisticsCollector::clear() {
    freqStatsMap.clear();
    recentSamples.clear();
    rssiWindows.clear();
}

size_t StatisticsCollector::getRecentSampleCount() const {
    return recentSamples.size();
}

size_t StatisticsCollector::getFrequencyCount() const {
    return freqStatsMap.size();
}
