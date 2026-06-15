#include <Arduino.h>
#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

// SH1107 128x128 OLED
U8G2_SH1107_128X128_F_HW_I2C u8g2(
    U8G2_R0,
    U8X8_PIN_NONE
);

// ===== Stats =====
int cpu_usage = 0;
float used_memory = 0.0f;
int total_memory = 0;
unsigned long uptime_days = 0;
unsigned long uptime_hours = 0;

// Serial buffer
char serialBuffer[128];

// Display state
bool displayNeedsUpdate = true;
unsigned long lastPacketTime = 0;

// Burn-in protection
int screenOffsetX = 0;
int screenOffsetY = 0;

// Timeouts
const unsigned long STALE_TIMEOUT = 15UL * 60UL * 1000UL;
const unsigned long OLED_SLEEP_TIMEOUT = 60UL * 60UL * 1000UL;


// ===== DRAW SCREEN (128x128 layout) =====
void drawScreen()
{
    bool stale = (millis() - lastPacketTime) > STALE_TIMEOUT;

    u8g2.setFontMode(1);
    u8g2.setBitmapMode(1);

    // ===== CPU =====
    u8g2.setFont(u8g2_font_t0_17b_tr);
    u8g2.drawStr(10 + screenOffsetX, 20 + screenOffsetY, "CPU");

    char cpuStr[8];
    snprintf(cpuStr, sizeof(cpuStr), "%d", cpu_usage);
    u8g2.drawStr(10 + screenOffsetX, 45 + screenOffsetY, cpuStr);

    u8g2.drawStr(40 + screenOffsetX, 45 + screenOffsetY, "%");

    // bar
    int barWidth = constrain(cpu_usage, 0, 100) * 60 / 100;
    u8g2.drawFrame(10 + screenOffsetX, 55 + screenOffsetY, 60, 10);
    u8g2.drawBox(10 + screenOffsetX, 55 + screenOffsetY, barWidth, 10);

    // ===== RAM =====
    u8g2.drawStr(10 + screenOffsetX, 85 + screenOffsetY, "RAM");

    char ramStr[32];
    snprintf(ramStr, sizeof(ramStr), "%.1f / %d GB", used_memory, total_memory);
    u8g2.drawStr(10 + screenOffsetX, 105 + screenOffsetY, ramStr);

    // ===== UPTIME =====
    u8g2.drawStr(70 + screenOffsetX, 20 + screenOffsetY, "UP");

    char up1[16];
    char up2[16];

    snprintf(up1, sizeof(up1), "%lud", uptime_days);
    snprintf(up2, sizeof(up2), "%luh", uptime_hours);

    u8g2.drawStr(70 + screenOffsetX, 45 + screenOffsetY, up1);
    u8g2.drawStr(70 + screenOffsetX, 65 + screenOffsetY, up2);

    // ===== STALE WARNING =====
    if (stale)
    {
        u8g2.setFont(u8g2_font_6x10_tr);
        u8g2.drawStr(70 + screenOffsetX, 120 + screenOffsetY, "STALE");
    }
}


// ===== PARSER =====
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

    cpu_usage = constrain(cpu, 0, 100);
    used_memory = ramUsed;
    total_memory = ramTotal;
    uptime_days = days;
    uptime_hours = hours;

    return true;
}


// ===== SETUP =====
void setup()
{
    Serial.begin(115200);
    Serial.setTimeout(50);

    u8g2.begin();

    lastPacketTime = millis();
}


// ===== LOOP =====
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

            // pixel shift
            screenOffsetX = (screenOffsetX + 1) % 3;
            screenOffsetY = (screenOffsetY + 1) % 3;

            displayNeedsUpdate = true;
        }
    }

    // power save after 1 hour
    if (millis() - lastPacketTime > OLED_SLEEP_TIMEOUT)
        u8g2.setPowerSave(1);
    else
        u8g2.setPowerSave(0);

    // redraw only on update
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
