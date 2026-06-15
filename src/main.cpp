#include <Arduino.h>
#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

// SSD1306 128x64 OLED
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
    U8G2_R0,
    U8X8_PIN_NONE);

// Icons
static const unsigned char image_BAT_percent_bits[] U8X8_PROGMEM = {
    0x0c,0x03,0x1e,0x03,0x33,0x03,0xb3,0x03,
    0xde,0x01,0xec,0x00,0x70,0x00,0x38,0x00,
    0xdc,0x00,0xee,0x01,0x37,0x03,0x33,0x03,
    0xe3,0x01,0xc3,0x00
};

static const unsigned char image_CPU_bar_bits[] U8X8_PROGMEM = {
    0xfc,0xff,0xff,0xff,0x01,
    0x02,0x00,0x00,0x00,0x02,
    0x01,0x00,0x00,0x00,0x04,
    0x01,0x00,0x00,0x00,0x04,
    0x01,0x00,0x00,0x00,0x04,
    0x01,0x00,0x00,0x00,0x04,
    0x02,0x00,0x00,0x00,0x02,
    0xfc,0xff,0xff,0xff,0x01
};

// Current stats
int cpu_usage = 0;
float used_memory = 0.0f;
int total_memory = 0;
unsigned long uptime_days = 0;
unsigned long uptime_hours = 0;

// Serial buffer
char serialBuffer[128];

// Display state
bool displayNeedsUpdate = true;

// Last valid packet
unsigned long lastPacketTime = 0;

// Burn-in protection
int screenOffsetX = 0;
int screenOffsetY = 0;

// Timeouts
const unsigned long STALE_TIMEOUT =
    15UL * 60UL * 1000UL;   // 15 min

const unsigned long OLED_SLEEP_TIMEOUT =
    60UL * 60UL * 1000UL;   // 1 hour


void drawScreen()
{
    bool stale =
        (millis() - lastPacketTime) >
        STALE_TIMEOUT;

    u8g2.setFontMode(1);
    u8g2.setBitmapMode(1);
    u8g2.setFont(u8g2_font_t0_17b_tr);

    // CPU
    u8g2.drawStr(
        7 + screenOffsetX,
        16 + screenOffsetY,
        "CPU");

    char cpuStr[8];
    snprintf(cpuStr, sizeof(cpuStr), "%d", cpu_usage);

    u8g2.drawStr(
        15 + screenOffsetX,
        38 + screenOffsetY,
        cpuStr);

    u8g2.drawXBMP(
        3 + screenOffsetX,
        25 + screenOffsetY,
        10,
        14,
        image_BAT_percent_bits);

    u8g2.drawXBMP(
        4 + screenOffsetX,
        47 + screenOffsetY,
        35,
        9,
        image_CPU_bar_bits);

    int barWidth =
        constrain(cpu_usage, 0, 100) * 31 / 100;

    u8g2.drawBox(
        6 + screenOffsetX,
        49 + screenOffsetY,
        barWidth,
        5);

    // RAM
    u8g2.drawStr(
        51 + screenOffsetX,
        16 + screenOffsetY,
        "RAM");

    char ramUsedStr[16];
    snprintf(
        ramUsedStr,
        sizeof(ramUsedStr),
        "%.1f",
        used_memory);

    u8g2.drawStr(
        46 + screenOffsetX,
        37 + screenOffsetY,
        ramUsedStr);

    u8g2.drawStr(
        75 + screenOffsetX,
        37 + screenOffsetY,
        "/");

    char ramTotalStr[16];
    snprintf(
        ramTotalStr,
        sizeof(ramTotalStr),
        "%d",
        total_memory);

    u8g2.drawStr(
        46 + screenOffsetX,
        55 + screenOffsetY,
        ramTotalStr);

    u8g2.drawStr(
        65 + screenOffsetX,
        55 + screenOffsetY,
        "GB");

    // Uptime
    u8g2.drawStr(
        88 + screenOffsetX,
        16 + screenOffsetY,
        "UP");

    char daysStr[12];
    snprintf(
        daysStr,
        sizeof(daysStr),
        "%luD",
        uptime_days);

    u8g2.drawStr(
        90 + screenOffsetX,
        35 + screenOffsetY,
        daysStr);

    char hoursStr[12];
    snprintf(
        hoursStr,
        sizeof(hoursStr),
        "%luH",
        uptime_hours);

    u8g2.drawStr(
        90 + screenOffsetX,
        55 + screenOffsetY,
        hoursStr);

    // Stale indicator
    if (stale)
    {
        u8g2.setFont(u8g2_font_5x8_tr);

        u8g2.drawStr(
            85 + screenOffsetX,
            63 + screenOffsetY,
            "STALE");
    }
}


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
        &hours);

    if (parsed != 5)
    {
        return false;
    }

    cpu_usage = constrain(cpu, 0, 100);
    used_memory = ramUsed;
    total_memory = ramTotal;
    uptime_days = days;
    uptime_hours = hours;

    return true;
}


void setup()
{
    Serial.begin(115200);
    Serial.setTimeout(50);

    u8g2.begin();

    lastPacketTime = millis();

    u8g2.firstPage();
    do
    {
        drawScreen();
    }
    while (u8g2.nextPage());
}


void loop()
{
    // Handle serial input
    if (Serial.available())
    {
        size_t len = Serial.readBytesUntil(
            '\n',
            serialBuffer,
            sizeof(serialBuffer) - 1);

        serialBuffer[len] = '\0';

        if (parseData(serialBuffer))
        {
            lastPacketTime = millis();

            // Move display slightly on every update
            screenOffsetX =
                (screenOffsetX + 1) % 3;

            screenOffsetY =
                (screenOffsetY + 1) % 3;

            displayNeedsUpdate = true;
        }
    }

    // OLED power save after 1 hour
    if ((millis() - lastPacketTime) >
        OLED_SLEEP_TIMEOUT)
    {
        u8g2.setPowerSave(1);
    }
    else
    {
        u8g2.setPowerSave(0);
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
