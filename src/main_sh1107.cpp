#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

U8G2_SH1107_128X128_F_HW_I2C u8g2(
    U8G2_R0,
    U8X8_PIN_NONE
);

// ================= DATA =================
int cpu_usage = 0;
float ram_used = 0;
int ram_total = 0;
unsigned long uptime_days = 0;
unsigned long uptime_hours = 0;

// smoothing
float cpu_smooth = 0;
float ram_smooth = 0;

// CPU history graph
#define GRAPH_W 80
#define GRAPH_H 30
uint8_t cpu_history[GRAPH_W];
uint8_t history_index = 0;

// serial buffer
char serialBuffer[128];

// state
bool displayNeedsUpdate = true;
unsigned long lastPacketTime = 0;

// burn-in protection
int offsetX = 0;
int offsetY = 0;

// timeouts
const unsigned long STALE_TIMEOUT = 15UL * 60UL * 1000UL;
const unsigned long SLEEP_TIMEOUT  = 60UL * 60UL * 1000UL;


// ================= PARSER =================
bool parseData(char *data)
{
    int cpu;
    float ramU;
    int ramT;
    unsigned long d;
    unsigned long h;

    int ok = sscanf(
        data,
        "CPU:%dRAM_USED:%fRAM_TOTAL:%dUPTIME_D:%luUPTIME_H:%lu",
        &cpu, &ramU, &ramT, &d, &h
    );

    if (ok != 5) return false;

    cpu = constrain(cpu, 0, 100);

    // smoothing
    cpu_smooth = cpu_smooth * 0.7 + cpu * 0.3;
    ram_smooth = ram_smooth * 0.7 + ramU * 0.3;

    cpu_usage = cpu;
    ram_used = ram_smooth;
    ram_total = ramT;
    uptime_days = d;
    uptime_hours = h;

    // update graph
    cpu_history[history_index] = cpu;
    history_index = (history_index + 1) % GRAPH_W;

    return true;
}


// ================= DRAW GRAPH =================
void drawGraph(int x, int y)
{
    for (int i = 0; i < GRAPH_W; i++)
    {
        int idx = (history_index + i) % GRAPH_W;
        int val = cpu_history[idx];

        int h = (val * GRAPH_H) / 100;

        u8g2.drawVLine(x + i, y + (GRAPH_H - h), h);
    }
}


// ================= DRAW SCREEN =================
void drawScreen()
{
    bool stale = (millis() - lastPacketTime) > STALE_TIMEOUT;

    u8g2.setFont(u8g2_font_6x10_tr);

    // ===== CPU =====
    u8g2.drawStr(5 + offsetX, 12 + offsetY, "CPU");

    char cpuStr[8];
    snprintf(cpuStr, sizeof(cpuStr), "%d%%", cpu_usage);
    u8g2.drawStr(5 + offsetX, 25 + offsetY, cpuStr);

    u8g2.drawFrame(5 + offsetX, 30 + offsetY, 50, 6);
    u8g2.drawBox(5 + offsetX, 30 + offsetY,
                 cpu_usage / 2, 6);

    // ===== RAM =====
    u8g2.drawStr(5 + offsetX, 50 + offsetY, "RAM");

    char ramStr[24];
    snprintf(ramStr, sizeof(ramStr),
             "%.1f / %d GB",
             ram_used, ram_total);

    u8g2.drawStr(5 + offsetX, 62 + offsetY, ramStr);

    // ===== UPTIME =====
    char upStr[32];
    snprintf(upStr, sizeof(upStr),
             "%lud %luh",
             uptime_days,
             uptime_hours);

    u8g2.drawStr(70 + offsetX, 15 + offsetY, "UP");
    u8g2.drawStr(70 + offsetX, 28 + offsetY, upStr);

    // ===== GRAPH =====
    drawGraph(10 + offsetX, 80 + offsetY);

    // stale warning
    if (stale)
    {
        u8g2.drawStr(80 + offsetX, 120 + offsetY, "STALE");
    }
}


// ================= SETUP =================
void setup()
{
    Wire.begin(8, 9);   // ESP32-C3 SuperMini
    Serial.begin(115200);
    delay(1000);

    u8g2.begin();
    u8g2.setBusClock(400000);

    // init graph
    for (int i = 0; i < GRAPH_W; i++)
        cpu_history[i] = 0;

    lastPacketTime = millis();
}


// ================= LOOP =================
void loop()
{
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

            offsetX = (offsetX + 1) % 3;
            offsetY = (offsetY + 1) % 3;

            displayNeedsUpdate = true;
        }
    }

    // power save
    static bool sleep = false;

    if (!sleep && millis() - lastPacketTime > SLEEP_TIMEOUT)
    {
        u8g2.setPowerSave(1);
        sleep = true;
    }

    if (sleep && millis() - lastPacketTime <= SLEEP_TIMEOUT)
    {
        u8g2.setPowerSave(0);
        sleep = false;
    }

    if (displayNeedsUpdate)
    {
        u8g2.firstPage();
        do
        {
            drawScreen();
        } while (u8g2.nextPage());

        displayNeedsUpdate = false;
    }

    delay(100);
}
