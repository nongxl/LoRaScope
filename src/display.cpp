#include "display.h"
#include <M5Cardputer.h>
#include <map>

ScopeDisplay::ScopeDisplay()
    : canvas(nullptr), canvasSystemBar(nullptr), currentMode(MODE_RADAR),
      batteryPct(100), currentRssi(-120), isScanning(false),
      moduleName("LoRa"), currentFreq(0), currentFreqIndex(0), totalFreqCount(0) {
}

ScopeDisplay::~ScopeDisplay() {
    if (canvas) {
        delete canvas;
    }
    if (canvasSystemBar) {
        delete canvasSystemBar;
    }
}

bool ScopeDisplay::init() {
    M5Cardputer.Display.init();
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.fillScreen(BG_COLOR);
    
    canvas = new M5Canvas(&M5Cardputer.Display);
    canvasSystemBar = new M5Canvas(&M5Cardputer.Display);
    
    if (!canvas->createSprite(ww, wh) || !canvasSystemBar->createSprite(sw, sh)) {
        return false;
    }
    
    return true;
}

void ScopeDisplay::update(const std::vector<RadarPoint>& points, const EventStats& stats) {
    drawSystemBar();
    
    switch (currentMode) {
        case MODE_TIMELINE:
            drawTimeline(points, stats);
            break;
        case MODE_HISTOGRAM:
            drawHistogram(points, stats);
            break;
        case MODE_EVENTLIST:
            drawEventList(points, stats);
            break;
        case MODE_STATISTICS:
            drawStatistics(points, stats);
            break;
        case MODE_FREQCOMPARE:
            drawFreqCompare(points, stats);
            break;
        case MODE_REALTIME:
            drawRealtimeMonitor(points, stats);
            break;
        case MODE_RADAR:
            drawRadar(points, stats);
            break;
    }
}

void ScopeDisplay::setMode(DisplayMode mode) {
    currentMode = mode;
}

DisplayMode ScopeDisplay::getMode() const {
    return currentMode;
}

void ScopeDisplay::setBatteryPct(uint8_t pct) {
    batteryPct = pct;
}

void ScopeDisplay::setCurrentRssi(int rssi) {
    currentRssi = rssi;
}

void ScopeDisplay::setScanning(bool scanning) {
    isScanning = scanning;
}

void ScopeDisplay::setModuleName(const String& name) {
    moduleName = name;
}

void ScopeDisplay::setCurrentFreq(uint32_t freq) {
    currentFreq = freq;
}

void ScopeDisplay::setCurrentFreqIndex(uint8_t index, uint8_t total) {
    currentFreqIndex = index;
    totalFreqCount = total;
}

void ScopeDisplay::setFrequencies(const std::vector<uint32_t>& freqs) {
    freqList = freqs;
}

void ScopeDisplay::drawSystemBar() {
    canvasSystemBar->fillSprite(BG_COLOR);
    canvasSystemBar->fillRoundRect(sx + m, sy, sw - 2 * m, sh - m, 3, UX_COLOR_DARK);
    canvasSystemBar->fillRect(sx + m, sy, sw - 2 * m, 3, UX_COLOR_DARK);
    
    canvasSystemBar->setTextColor(COLOR_SILVER, UX_COLOR_DARK);
    canvasSystemBar->setTextSize(1);
    canvasSystemBar->setTextDatum(middle_left);
    canvasSystemBar->drawString(moduleName, sx + 3 * m, sy + sh / 2);
    
    canvasSystemBar->setTextDatum(middle_center);
    String freqStr = currentFreq > 0 ? String(currentFreq / 1000000.0, 2) + " MHz" : "--- MHz";
    canvasSystemBar->drawString(freqStr, sw / 2, sy + sh / 2);
    
    //if (totalFreqCount > 1) {
    //    String indexStr = String(currentFreqIndex + 1) + "/" + String(totalFreqCount);
    //    canvasSystemBar->setTextDatum(middle_right);
    //    canvasSystemBar->drawString(indexStr, sw - 65, sy + sh / 2);
    //}
    
    draw_scan_icon(canvasSystemBar, sw - 75, sy + sh / 2 - 1, isScanning);
    draw_rssi_indicator(canvasSystemBar, sw - 60, sy + sh / 2 - 5, currentRssi, true);
    draw_battery_indicator(canvasSystemBar, sw - 25, sy + sh / 2 - 5, batteryPct);
    
    canvasSystemBar->pushSprite(sx, sy);
}

void ScopeDisplay::drawTimeline(const std::vector<RadarPoint>& points, const EventStats& stats) {
    canvas->fillSprite(BG_COLOR);
    
    canvas->setTextColor(COLOR_SILVER);
    canvas->setTextDatum(top_center);
    canvas->drawString("Timeline", ww / 2, 2 * m);
    
    for (int i = 0; i <= 1; i++) {
        canvas->drawLine(10, 3 * m + canvas->fontHeight() + i, ww - 10, 3 * m + canvas->fontHeight() + i, UX_COLOR_LIGHT);
    }
    
    if (points.empty()) {
        canvas->setTextDatum(middle_center);
        canvas->drawString("No events", ww / 2, wh / 2);
        canvas->pushSprite(wx, wy);
        return;
    }
    
    uint32_t now = millis();
    uint32_t timeWindow = 60000;
    int16_t minRssi = -120;
    int16_t maxRssi = -50;
    
    int graphX = 2 * m;
    int graphY = 4 * m + canvas->fontHeight();
    int graphW = ww - 4 * m;
    int graphH = wh - graphY - 2 * m;
    
    canvas->drawRect(graphX, graphY, graphW, graphH, UX_COLOR_LIGHT);
    
    canvas->setTextDatum(top_left);
    canvas->setTextSize(1);
    canvas->setTextColor(COLOR_SILVER);
    canvas->drawString("-50", graphX + graphW + 2, graphY);
    canvas->drawString("-120", graphX + graphW + 2, graphY + graphH - canvas->fontHeight());
    
    for (const auto& point : points) {
        uint32_t timeDiff = now - point.timestamp;
        if (timeDiff > timeWindow) continue;
        
        float x = graphX + graphW * (1.0 - (float)timeDiff / timeWindow);
        float y = graphY + graphH * (1.0 - (float)(point.rssi - minRssi) / (maxRssi - minRssi));
        
        uint16_t color = (point.eventType == EVENT_RX_DONE) ? TFT_GREEN : TFT_RED;
        int radius = (point.packetLength > 0) ? 4 : 2;
        
        canvas->fillCircle(x, y, radius, color);
    }
    
    canvas->pushSprite(wx, wy);
}

void ScopeDisplay::drawHistogram(const std::vector<RadarPoint>& points, const EventStats& stats) {
    canvas->fillSprite(BG_COLOR);
    
    canvas->setTextColor(COLOR_SILVER);
    canvas->setTextDatum(top_center);
    canvas->drawString("RSSI Histogram", ww / 2, 2 * m);
    
    for (int i = 0; i <= 1; i++) {
        canvas->drawLine(10, 3 * m + canvas->fontHeight() + i, ww - 10, 3 * m + canvas->fontHeight() + i, UX_COLOR_LIGHT);
    }
    
    if (points.empty()) {
        canvas->setTextDatum(middle_center);
        canvas->drawString("No events", ww / 2, wh / 2);
        canvas->pushSprite(wx, wy);
        return;
    }
    
    const int numBins = 10;
    int bins[numBins] = {0};
    int16_t minRssi = -120;
    int16_t maxRssi = -50;
    int binSize = (maxRssi - minRssi) / numBins;
    
    for (const auto& point : points) {
        int binIndex = (point.rssi - minRssi) / binSize;
        if (binIndex < 0) binIndex = 0;
        if (binIndex >= numBins) binIndex = numBins - 1;
        bins[binIndex]++;
    }
    
    int maxCount = 0;
    for (int i = 0; i < numBins; i++) {
        if (bins[i] > maxCount) maxCount = bins[i];
    }
    
    int startY = 4 * m + canvas->fontHeight();
    int barWidth = (ww - 4 * m) / numBins;
    int maxBarHeight = wh - startY - 2 * m;
    
    for (int i = 0; i < numBins; i++) {
        int x = 2 * m + i * barWidth;
        int barHeight = (maxCount > 0) ? (maxBarHeight * bins[i] / maxCount) : 0;
        int y = startY + maxBarHeight - barHeight;
        
        int16_t binRssi = minRssi + i * binSize;
        uint16_t color = (binRssi > -80) ? TFT_GREEN : (binRssi > -100) ? TFT_YELLOW : TFT_RED;
        
        canvas->fillRect(x + 1, y, barWidth - 2, barHeight, color);
        
        if (i % 2 == 0) {
            canvas->setTextDatum(bottom_center);
            canvas->setTextSize(1);
            canvas->setTextColor(COLOR_SILVER);
            canvas->drawString(String(binRssi), x + barWidth / 2, wh - 2 * m);
        }
    }
    
    canvas->pushSprite(wx, wy);
}

void ScopeDisplay::drawEventList(const std::vector<RadarPoint>& points, const EventStats& stats) {
    canvas->fillSprite(BG_COLOR);
    
    canvas->setTextColor(COLOR_SILVER);
    canvas->setTextDatum(top_center);
    canvas->drawString("Event List", ww / 2, 2 * m);
    
    for (int i = 0; i <= 1; i++) {
        canvas->drawLine(10, 3 * m + canvas->fontHeight() + i, ww - 10, 3 * m + canvas->fontHeight() + i, UX_COLOR_LIGHT);
    }
    
    if (points.empty()) {
        canvas->setTextDatum(middle_center);
        canvas->drawString("No events", ww / 2, wh / 2);
        canvas->pushSprite(wx, wy);
        return;
    }
    
    int startY = 4 * m + canvas->fontHeight();
    int lineHeight = canvas->fontHeight() + 2;
    int maxLines = (wh - startY) / lineHeight;
    
    int startIdx = std::max(0, (int)points.size() - maxLines);
    
    for (int i = startIdx; i < (int)points.size(); i++) {
        int y = startY + (i - startIdx) * lineHeight;
        const auto& point = points[i];
        
        uint16_t color = (point.eventType == EVENT_RX_DONE) ? TFT_GREEN : TFT_RED;
        
        canvas->fillRect(2 * m, y, 4, lineHeight - 1, color);
        
        canvas->setTextDatum(top_left);
        canvas->setTextSize(1);
        canvas->setTextColor(COLOR_SILVER);
        
        String line = "RSSI:" + String(point.rssi) + " ";
        line += "Len:" + String(point.packetLength) + " ";
        line += "T:" + String((millis() - point.timestamp) / 1000) + "s";
        
        canvas->drawString(line, 2 * m + 6, y);
    }
    
    canvas->pushSprite(wx, wy);
}

void ScopeDisplay::drawStatistics(const std::vector<RadarPoint>& points, const EventStats& stats) {
    canvas->fillSprite(BG_COLOR);
    
    canvas->setTextColor(COLOR_SILVER);
    canvas->setTextDatum(top_center);
    canvas->drawString("Statistics", ww / 2, 2 * m);
    
    for (int i = 0; i <= 1; i++) {
        canvas->drawLine(10, 3 * m + canvas->fontHeight() + i, ww - 10, 3 * m + canvas->fontHeight() + i, UX_COLOR_LIGHT);
    }
    
    if (stats.totalEvents == 0) {
        canvas->setTextDatum(middle_center);
        canvas->drawString("No events", ww / 2, wh / 2);
        canvas->pushSprite(wx, wy);
        return;
    }
    
    int y = 4 * m + canvas->fontHeight();
    int lineHeight = canvas->fontHeight() + 4;
    
    canvas->setTextDatum(top_left);
    canvas->setTextSize(1);
    
    canvas->setTextColor(UX_COLOR_ACCENT);
    canvas->drawString("Total Events:", 2 * m, y);
    canvas->setTextColor(COLOR_SILVER);
    canvas->drawString(String(stats.totalEvents), 2 * m + 80, y);
    
    y += lineHeight;
    canvas->setTextColor(UX_COLOR_ACCENT);
    canvas->drawString("RX Done:", 2 * m, y);
    canvas->setTextColor(TFT_GREEN);
    canvas->drawString(String(stats.rxDoneCount), 2 * m + 80, y);
    
    y += lineHeight;
    canvas->setTextColor(UX_COLOR_ACCENT);
    canvas->drawString("RX Error:", 2 * m, y);
    canvas->setTextColor(TFT_RED);
    canvas->drawString(String(stats.rxErrorCount), 2 * m + 80, y);
    
    y += lineHeight;
    canvas->setTextColor(UX_COLOR_ACCENT);
    canvas->drawString("Avg RSSI:", 2 * m, y);
    canvas->setTextColor(COLOR_SILVER);
    canvas->drawString(String(stats.avgRssi) + " dBm", 2 * m + 80, y);
    
    y += lineHeight;
    canvas->setTextColor(UX_COLOR_ACCENT);
    canvas->drawString("Max RSSI:", 2 * m, y);
    canvas->setTextColor(COLOR_SILVER);
    canvas->drawString(String(stats.maxRssi) + " dBm", 2 * m + 80, y);
    
    y += lineHeight;
    canvas->setTextColor(UX_COLOR_ACCENT);
    canvas->drawString("Min RSSI:", 2 * m, y);
    canvas->setTextColor(COLOR_SILVER);
    canvas->drawString(String(stats.minRssi) + " dBm", 2 * m + 80, y);
    
    y += lineHeight;
    canvas->setTextColor(UX_COLOR_ACCENT);
    canvas->drawString("Success Rate:", 2 * m, y);
    float successRate = (float)stats.rxDoneCount / stats.totalEvents * 100.0;
    canvas->setTextColor(COLOR_SILVER);
    canvas->drawString(String(successRate, 1) + "%", 2 * m + 80, y);
    
    canvas->pushSprite(wx, wy);
}

void ScopeDisplay::drawActivityIndicator(int x, int y, float score) {
    uint16_t color = getScoreColor(score);
    canvas->fillCircle(x, y, 5, color);
}

uint16_t ScopeDisplay::getScoreColor(float score) {
    if (score < 0.3) {
        return TFT_GREEN;
    } else if (score < 0.6) {
        return TFT_YELLOW;
    } else {
        return TFT_RED;
    }
}

void ScopeDisplay::draw_tx_indicator(M5Canvas* c, int x, int y) {
    c->fillTriangle(x, y, x + 6, y - 4, x + 6, y + 4, TFT_GREEN);
}

void ScopeDisplay::draw_rx_indicator(M5Canvas* c, int x, int y) {
    c->fillTriangle(x, y, x - 6, y - 4, x - 6, y + 4, TFT_BLUE);
}

void ScopeDisplay::draw_rssi_indicator(M5Canvas* c, int x, int y, int rssi, bool show) {
    if (!show) return;
    
    c->fillRect(x, y, 20, 8, UX_COLOR_DARK);
    
    int bars = 0;
    if (rssi > -70) bars = 4;
    else if (rssi > -80) bars = 3;
    else if (rssi > -90) bars = 2;
    else if (rssi > -100) bars = 1;
    
    uint16_t color = bars >= 3 ? TFT_GREEN : (bars >= 2 ? TFT_YELLOW : TFT_RED);
    
    for (int i = 0; i < bars; i++) {
        int barHeight = 3 + i * 2;
        c->fillRect(x + i * 5, y + 8 - barHeight, 4, barHeight, color);
    }
}

void ScopeDisplay::draw_battery_indicator(M5Canvas* c, int x, int y, int pct) {
    c->drawRect(x, y, 20, 8, COLOR_SILVER);
    c->fillRect(x + 20, y + 2, 2, 4, COLOR_SILVER);
    
    int fillWidth = (pct * 18) / 100;
    uint16_t color = (pct > 50) ? TFT_GREEN : (pct > 20) ? TFT_YELLOW : TFT_RED;
    c->fillRect(x + 1, y + 1, fillWidth, 6, color);
}

void ScopeDisplay::draw_scan_icon(M5Canvas* c, int x, int y, bool active) {
    if (active) {
        c->fillCircle(x, y, 6, UX_COLOR_ACCENT);
        c->drawCircle(x, y, 4, BG_COLOR);
        c->fillCircle(x, y, 2, UX_COLOR_ACCENT);
    } else {
        c->drawCircle(x, y, 6, UX_COLOR_LIGHT);
        c->drawCircle(x, y, 4, BG_COLOR);
        c->drawCircle(x, y, 2, UX_COLOR_LIGHT);
    }
}

void ScopeDisplay::draw_timeline_icon(M5Canvas* c, int x, int y, bool active) {
    uint16_t color = active ? UX_COLOR_ACCENT : UX_COLOR_LIGHT;
    c->drawLine(x, y, x + 10, y, color);
    c->drawLine(x + 5, y - 5, x + 5, y + 5, color);
}

void ScopeDisplay::draw_histogram_icon(M5Canvas* c, int x, int y, bool active) {
    uint16_t color = active ? UX_COLOR_ACCENT : UX_COLOR_LIGHT;
    c->fillRect(x, y + 4, 3, 6, color);
    c->fillRect(x + 4, y + 2, 3, 8, color);
    c->fillRect(x + 8, y, 3, 10, color);
}

void ScopeDisplay::draw_eventlist_icon(M5Canvas* c, int x, int y, bool active) {
    uint16_t color = active ? UX_COLOR_ACCENT : UX_COLOR_LIGHT;
    c->drawLine(x, y, x + 10, y, color);
    c->drawLine(x, y + 3, x + 10, y + 3, color);
    c->drawLine(x, y + 6, x + 10, y + 6, color);
}

void ScopeDisplay::draw_statistics_icon(M5Canvas* c, int x, int y, bool active) {
    uint16_t color = active ? UX_COLOR_ACCENT : UX_COLOR_LIGHT;
    c->drawCircle(x, y, 4, color);
    c->drawLine(x + 4, y, x + 8, y - 4, color);
    c->drawLine(x + 4, y, x + 8, y + 4, color);
}

void ScopeDisplay::drawFreqCompare(const std::vector<RadarPoint>& points, const EventStats& stats) {
    canvas->fillSprite(BG_COLOR);
    
    canvas->setTextColor(COLOR_SILVER);
    canvas->setTextDatum(top_center);
    canvas->drawString("Freq Compare", ww / 2, 2 * m);
    
    for (int i = 0; i <= 1; i++) {
        canvas->drawLine(10, 3 * m + canvas->fontHeight() + i, ww - 10, 3 * m + canvas->fontHeight() + i, UX_COLOR_LIGHT);
    }
    
    if (totalFreqCount == 0) {
        canvas->setTextDatum(middle_center);
        canvas->drawString("No frequencies", ww / 2, wh / 2);
        canvas->pushSprite(wx, wy);
        return;
    }
    
    int startY = 4 * m + canvas->fontHeight();
    int lineHeight = canvas->fontHeight() + 2;
    int maxLines = (wh - startY) / lineHeight;
    
    int startIdx = std::max(0, currentFreqIndex - maxLines / 2);
    int endIdx = std::min((int)totalFreqCount, startIdx + maxLines);
    
    for (int i = startIdx; i < endIdx; i++) {
        int y = startY + (i - startIdx) * lineHeight;
        
        String freqStr;
        if (i < freqList.size()) {
            freqStr = String(freqList[i] / 1000000.0, 2) + " MHz";
        } else {
            freqStr = "Freq " + String(i + 1);
        }
        
        uint16_t color = (i == currentFreqIndex) ? UX_COLOR_ACCENT : COLOR_SILVER;
        
        canvas->setTextDatum(top_left);
        canvas->setTextSize(1);
        canvas->setTextColor(color);
        
        canvas->drawString(freqStr, 2 * m, y);
        
        if (i == currentFreqIndex) {
            canvas->drawRect(2 * m, y - 1, ww - 4 * m, lineHeight, UX_COLOR_ACCENT);
        }
    }
    
    canvas->pushSprite(wx, wy);
}

void ScopeDisplay::drawRealtimeMonitor(const std::vector<RadarPoint>& points, const EventStats& stats) {
    canvas->fillSprite(BG_COLOR);
    
    canvas->setTextColor(COLOR_SILVER);
    canvas->setTextDatum(top_center);
    canvas->drawString("Realtime Monitor", ww / 2, 2 * m);
    
    for (int i = 0; i <= 1; i++) {
        canvas->drawLine(10, 3 * m + canvas->fontHeight() + i, ww - 10, 3 * m + canvas->fontHeight() + i, UX_COLOR_LIGHT);
    }
    
    int startY = 4 * m + canvas->fontHeight();
    int graphH = wh - startY - 2 * m;
    int graphX = 2 * m;
    int graphY = startY;
    int graphW = ww - 4 * m;
    
    canvas->drawRect(graphX, graphY, graphW, graphH, UX_COLOR_LIGHT);
    
    canvas->setTextDatum(top_left);
    canvas->setTextSize(1);
    canvas->setTextColor(COLOR_SILVER);
    canvas->drawString("-50", graphX + graphW + 2, graphY);
    canvas->drawString("-120", graphX + graphW + 2, graphY + graphH - canvas->fontHeight());
    
    if (points.empty()) {
        canvas->setTextDatum(middle_center);
        canvas->drawString("No data", ww / 2, wh / 2);
        canvas->pushSprite(wx, wy);
        return;
    }
    
    uint32_t now = millis();
    uint32_t timeWindow = 10000;
    int16_t minRssi = -120;
    int16_t maxRssi = -50;
    
    for (const auto& point : points) {
        uint32_t timeDiff = now - point.timestamp;
        if (timeDiff > timeWindow) continue;
        
        float x = graphX + graphW * (1.0 - (float)timeDiff / timeWindow);
        float y = graphY + graphH * (1.0 - (float)(point.rssi - minRssi) / (maxRssi - minRssi));
        
        uint16_t color = (point.eventType == EVENT_RX_DONE) ? TFT_GREEN : TFT_RED;
        int radius = (point.packetLength > 0) ? 3 : 2;
        
        canvas->fillCircle(x, y, radius, color);
    }
    
    canvas->setTextDatum(bottom_center);
    canvas->setTextSize(1);
    canvas->setTextColor(COLOR_SILVER);
    canvas->drawString("Last 10s", ww / 2, wh - 2 * m);
    
    canvas->pushSprite(wx, wy);
}

void ScopeDisplay::drawRadar(const std::vector<RadarPoint>& points, const EventStats& stats) {
    canvas->fillSprite(BG_COLOR);
    
    canvas->setTextColor(COLOR_SILVER);
    canvas->setTextDatum(top_center);
    canvas->drawString("Radar", ww / 2, 2 * m);
    
    for (int i = 0; i <= 1; i++) {
        canvas->drawLine(10, 3 * m + canvas->fontHeight() + i, ww - 10, 3 * m + canvas->fontHeight() + i, UX_COLOR_LIGHT);
    }
    
    int centerX = ww / 2;
    int centerY = (wy + wh / 2) - 5;
    int maxRadius = std::min(ww, wh) / 2 - 6 * m;
    
    canvas->drawCircle(centerX, centerY, maxRadius, UX_COLOR_LIGHT);
    canvas->drawCircle(centerX, centerY, maxRadius * 0.75, UX_COLOR_LIGHT);
    canvas->drawCircle(centerX, centerY, maxRadius * 0.5, UX_COLOR_LIGHT);
    canvas->drawCircle(centerX, centerY, maxRadius * 0.25, UX_COLOR_LIGHT);
    
    canvas->drawLine(centerX, centerY - maxRadius, centerX, centerY + maxRadius, UX_COLOR_LIGHT);
    canvas->drawLine(centerX - maxRadius, centerY, centerX + maxRadius, centerY, UX_COLOR_LIGHT);
    
    if (points.empty() || freqList.empty()) {
        canvas->setTextDatum(middle_center);
        canvas->drawString("No data", ww / 2, wh / 2);
        canvas->pushSprite(wx, wy);
        return;
    }
    
    uint32_t startFreq = freqList[0];
    uint32_t endFreq = freqList[freqList.size() - 1];
    uint32_t freqRange = endFreq - startFreq;
    
    if (freqRange == 0) {
        canvas->setTextDatum(middle_center);
        canvas->drawString("Single freq", ww / 2, wh / 2);
        canvas->pushSprite(wx, wy);
        return;
    }
    
    std::map<uint32_t, bool> freqHasData;
    for (const auto& point : points) {
        freqHasData[point.frequency] = true;
    }
    
    int16_t minRssi = -120;
    int16_t maxRssi = -50;
    
    for (const auto& point : points) {
        float angle = 0;
        
        if (freqList.size() > 1) {
            angle = ((float)(point.frequency - startFreq) / freqRange) * 2 * PI - PI / 2;
        }
        
        float rssiNorm = constrain((float)(point.rssi - minRssi) / (maxRssi - minRssi), 0.0, 1.0);
        float radius = maxRadius * (1.0 - rssiNorm);
        
        float x = centerX + radius * cos(angle);
        float y = centerY + radius * sin(angle);
        
        uint16_t color;
        if (point.eventType == EVENT_RX_DONE) {
            if (rssiNorm > 0.7) {
                color = TFT_GREEN;
            } else if (rssiNorm > 0.4) {
                color = TFT_YELLOW;
            } else {
                color = TFT_ORANGE;
            }
        } else {
            color = TFT_RED;
        }
        
        int pointSize = (point.packetLength > 0) ? 3 : 2;
        canvas->fillCircle(x, y, pointSize, color);
    }
    
    for (const auto& freq : freqList) {
        if (freqHasData.find(freq) != freqHasData.end()) {
            float angle = ((float)(freq - startFreq) / freqRange) * 2 * PI - PI / 2;
            float labelRadius = maxRadius + 10;
            float labelX = centerX + labelRadius * cos(angle);
            float labelY = centerY + labelRadius * sin(angle);
            
            String freqStr = String(freq / 1000000.0, 2);
            
            canvas->setTextSize(1);
            canvas->setTextColor(COLOR_SILVER);
            
            if (angle < -PI / 2 || angle > PI / 2) {
                canvas->setTextDatum(top_right);
            } else {
                canvas->setTextDatum(top_left);
            }
            
            canvas->drawString(freqStr, labelX, labelY);
        }
    }
    
    canvas->pushSprite(wx, wy);
}

void ScopeDisplay::draw_freqcompare_icon(M5Canvas* c, int x, int y, bool active) {
    uint16_t color = active ? UX_COLOR_ACCENT : UX_COLOR_LIGHT;
    c->drawRect(x, y - 4, 10, 8, color);
    c->drawLine(x, y, x + 10, y, color);
    c->drawLine(x, y + 4, x + 10, y + 4, color);
}

void ScopeDisplay::draw_realtime_icon(M5Canvas* c, int x, int y, bool active) {
    uint16_t color = active ? UX_COLOR_ACCENT : UX_COLOR_LIGHT;
    c->drawCircle(x, y, 4, color);
    c->drawCircle(x + 4, y, 4, color);
    c->drawCircle(x + 8, y, 4, color);
}

void ScopeDisplay::draw_radar_icon(M5Canvas* c, int x, int y, bool active) {
    uint16_t color = active ? UX_COLOR_ACCENT : UX_COLOR_LIGHT;
    c->drawCircle(x + 4, y, 4, color);
    c->drawCircle(x + 4, y, 2, color);
    c->drawLine(x + 4, y, x + 4, y - 4, color);
    c->drawLine(x + 4, y, x + 8, y, color);
}
