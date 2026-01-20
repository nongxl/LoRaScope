#ifndef DISPLAY_H
#define DISPLAY_H

#include "common.h"
#include <M5Cardputer.h>

class ScopeDisplay {
private:
    M5Canvas* canvas;
    M5Canvas* canvasSystemBar;
    DisplayMode currentMode;
    
    uint8_t batteryPct;
    int currentRssi;
    bool isScanning;
    String moduleName;
    uint32_t currentFreq;
    uint8_t currentFreqIndex;
    uint8_t totalFreqCount;
    std::vector<uint32_t> freqList;
    
    const uint8_t w = 240;
    const uint8_t h = 135;
    const uint8_t m = 2;
    
    const uint8_t sx = 0;
    const uint8_t sy = 0;
    const uint8_t sw = w;
    const uint8_t sh = 20;
    
    const uint8_t wx = 0;
    const uint8_t wy = sy + sh;
    const uint8_t ww = w;
    const uint8_t wh = h - wy;
    
public:
    ScopeDisplay();
    ~ScopeDisplay();
    
    bool init();
    void update(const std::vector<RadarPoint>& points, const EventStats& stats);
    void setMode(DisplayMode mode);
    DisplayMode getMode() const;
    
    void setBatteryPct(uint8_t pct);
    void setCurrentRssi(int rssi);
    void setScanning(bool scanning);
    void setModuleName(const String& name);
    void setCurrentFreq(uint32_t freq);
    void setCurrentFreqIndex(uint8_t index, uint8_t total);
    void setFrequencies(const std::vector<uint32_t>& freqs);
    
private:
    void drawSystemBar();
    void drawTimeline(const std::vector<RadarPoint>& points, const EventStats& stats);
    void drawHistogram(const std::vector<RadarPoint>& points, const EventStats& stats);
    void drawEventList(const std::vector<RadarPoint>& points, const EventStats& stats);
    void drawStatistics(const std::vector<RadarPoint>& points, const EventStats& stats);
    void drawFreqCompare(const std::vector<RadarPoint>& points, const EventStats& stats);
    void drawRealtimeMonitor(const std::vector<RadarPoint>& points, const EventStats& stats);
    void drawRadar(const std::vector<RadarPoint>& points, const EventStats& stats);
    
    void drawActivityIndicator(int x, int y, float score);
    uint16_t getScoreColor(float score);
    
    void draw_tx_indicator(M5Canvas* c, int x, int y);
    void draw_rx_indicator(M5Canvas* c, int x, int y);
    void draw_rssi_indicator(M5Canvas* c, int x, int y, int rssi, bool show);
    void draw_battery_indicator(M5Canvas* c, int x, int y, int pct);
    void draw_scan_icon(M5Canvas* c, int x, int y, bool active);
    void draw_timeline_icon(M5Canvas* c, int x, int y, bool active);
    void draw_histogram_icon(M5Canvas* c, int x, int y, bool active);
    void draw_eventlist_icon(M5Canvas* c, int x, int y, bool active);
    void draw_statistics_icon(M5Canvas* c, int x, int y, bool active);
    void draw_freqcompare_icon(M5Canvas* c, int x, int y, bool active);
    void draw_realtime_icon(M5Canvas* c, int x, int y, bool active);
    void draw_radar_icon(M5Canvas* c, int x, int y, bool active);
};

#endif // DISPLAY_H
