#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

// ============================================================
// Display
// ============================================================

U8G2_SH1107_128X128_F_HW_I2C u8g2(
    U8G2_R0,
    U8X8_PIN_NONE
);

// ============================================================
// Configuration
// ============================================================

const unsigned long PAGE_INTERVAL      = 60000UL;          // 60 sec
const unsigned long STALE_TIMEOUT      = 15UL * 60UL * 1000UL;
const unsigned long OLED_SLEEP_TIMEOUT = 60UL * 60UL * 1000UL;

const uint8_t GRAPH_W = 100;
const uint8_t GRAPH_H = 30;

// ============================================================
// Data
// ============================================================

int cpu_usage = 0;
float used_memory = 0.0f;
int total_memory = 0;

unsigned long uptime_days = 0;
unsigned long uptime_hours = 0;

// Smoothed values
float cpu_smooth = 0.0f;
float ram_smooth = 0.0f;

// CPU history
uint8_t cpu_history[GRAPH_W];
uint8_t history_index = 0;

// Serial
char serialBuffer[128];

// Display state
bool displayNeedsUpdate = true;

unsigned long lastPacketTime = 0;
unsigned long lastPageSwitch = 0;

uint8_t currentPage = 0;

// Burn-in protection
int screenOffsetX = 0;
int screenOffsetY = 0;

// OLED sleep tracking
bool oledSleeping = false;

// ============================================================
// Parsing
// ============================================================

bool parseData(char *data)
{
    int cpu;
    float ramUsed;
    int ramTotal;
    unsigned long days;
    unsigned long hours;

    int parsed = sscanf(
        data,
        "CPU:%dRAM_USED:%fRAM_TOTAL:%dUPTIME_D:%luUPTIME_H:%lu",
        &cpu,
        &ramUsed,
        &ramTotal,
        &days,
        &hours
    );

    if (parsed != 5)
        return false;

    cpu = constrain(cpu, 0, 100);

    // Smoothing
    cpu_smooth = cpu_smooth * 0.7f + cpu * 0.3f;
    ram_smooth = ram_smooth * 0.7f + ramUsed * 0.3f;

    cpu_usage = cpu;
    used_memory = ram_smooth;
    total_memory = ramTotal;

    uptime_days = days;
    uptime_hours = hours;

    // History graph
    cpu_history[history_index] = cpu;
    history_index = (history_index + 1) % GRAPH_W;

    return true;
}

// ============================================================
// Graph
// ============================================================

void drawGraph(int x, int y)
{
    u8g2.drawFrame(
        x - 1,
        y - 1,
        GRAPH_W + 2,
        GRAPH_H + 2
    );

    for (int i = 0; i < GRAPH_W; i++)
    {
        int idx = (history_index + i) % GRAPH_W;

        int value = cpu_history[idx];

        int h = (value * GRAPH_H) / 100;

        if (h > 0)
        {
            u8g2.drawVLine(
                x + i,
                y + GRAPH_H - h,
                h
            );
        }
    }
}

// ============================================================
// Page 1 - Performance
// ============================================================

void drawPagePerformance()
{
    char buf[32];

    u8g2.setFont(u8g2_font_6x10_tr);

    // CPU
    u8g2.drawStr(
        5 + screenOffsetX,
        12 + screenOffsetY,
        "CPU"
    );

    snprintf(
        buf,
        sizeof(buf),
        "%d%%",
        cpu_usage
    );

    u8g2.drawStr(
        5 + screenOffsetX,
        28 + screenOffsetY,
        buf
    );

    u8g2.drawFrame(
        5 + screenOffsetX,
        34 + screenOffsetY,
        50,
        8
    );

    u8g2.drawBox(
        5 + screenOffsetX,
        34 + screenOffsetY,
        cpu_usage / 2,
        8
    );

    // RAM
    u8g2.drawStr(
        5 + screenOffsetX,
        58 + screenOffsetY,
        "RAM"
    );

    snprintf(
        buf,
        sizeof(buf),
        "%.1f/%d GB",
        used_memory,
        total_memory
    );

    u8g2.drawStr(
        5 + screenOffsetX,
        74 + screenOffsetY,
        buf
    );

    // Graph
    drawGraph(
        10 + screenOffsetX,
        90 + screenOffsetY
    );

    u8g2.drawStr(
        105 + screenOffsetX,
        120 + screenOffsetY,
        "P1"
    );
}

// ============================================================
// Page 2 - System
// ============================================================

void drawPageSystem()
{
    char buf[32];

    bool stale =
        (millis() - lastPacketTime) >
        STALE_TIMEOUT;

    u8g2.setFont(u8g2_font_6x10_tr);

    u8g2.drawStr(
        5 + screenOffsetX,
        15 + screenOffsetY,
        "UPTIME"
    );

    snprintf(
        buf,
        sizeof(buf),
        "%lud %luh",
        uptime_days,
        uptime_hours
    );

    u8g2.drawStr(
        5 + screenOffsetX,
        35 + screenOffsetY,
        buf
    );

    u8g2.drawStr(
        5 + screenOffsetX,
        65 + screenOffsetY,
        "STATUS"
    );

    u8g2.drawStr(
        5 + screenOffsetX,
        85 + screenOffsetY,
        stale ? "STALE" : "ONLINE"
    );

    u8g2.drawStr(
        105 + screenOffsetX,
        120 + screenOffsetY,
        "P2"
    );
}

// ============================================================
// Main Draw
// ============================================================

void drawScreen()
{
    if (currentPage == 0)
    {
        drawPagePerformance();
    }
    else
    {
        drawPageSystem();
    }
}

// ============================================================
// Setup
// ============================================================

void setup()
{
    Wire.begin(8, 9);      // ESP32-C3 SuperMini
    Serial.begin(115200);

    delay(1000);

    u8g2.begin();
    u8g2.setBusClock(400000);

    memset(cpu_history, 0, sizeof(cpu_history));

    lastPacketTime = millis();
    lastPageSwitch = millis();

    displayNeedsUpdate = true;
}

// ============================================================
// Loop
// ============================================================

void loop()
{
    // Serial data
    if (Serial.available())
    {
        size_t len = Serial.readBytesUntil(
            '\n',
            serialBuffer,
            sizeof(serialBuffer) - 1
        );

        serialBuffer[len] = '\0';

        if (parseData(serialBuffer))
        {
            lastPacketTime = millis();

            // Burn-in protection
            screenOffsetX =
                (screenOffsetX + 1) % 3;

            screenOffsetY =
                (screenOffsetY + 1) % 3;

            displayNeedsUpdate = true;
        }
    }

    // Page switching
    if (millis() - lastPageSwitch > PAGE_INTERVAL)
    {
        currentPage =
            (currentPage + 1) % 2;

        lastPageSwitch = millis();

        displayNeedsUpdate = true;
    }

    // OLED sleep management
    if (!oledSleeping &&
        millis() - lastPacketTime >
        OLED_SLEEP_TIMEOUT)
    {
        u8g2.setPowerSave(1);
        oledSleeping = true;
    }

    if (oledSleeping &&
        millis() - lastPacketTime <=
        OLED_SLEEP_TIMEOUT)
    {
        u8g2.setPowerSave(0);
        oledSleeping = false;

        displayNeedsUpdate = true;
    }

    // Redraw only when needed
    if (displayNeedsUpdate)
    {
        u8g2.firstPage();
        do
        {
            drawScreen();
        }
        while (u8g2.nextPage());

        displayNeedsUpdate = false;
    }

    delay(100);
}
