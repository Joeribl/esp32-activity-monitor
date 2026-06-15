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

void drawScreen()
{
    u8g2.setFontMode(1);
    u8g2.setBitmapMode(1);
    u8g2.setFont(u8g2_font_t0_17b_tr);

    // CPU
    u8g2.drawStr(7, 16, "CPU");

    char cpuStr[8];
    snprintf(cpuStr, sizeof(cpuStr), "%d", cpu_usage);
    u8g2.drawStr(15, 38, cpuStr);

    u8g2.drawXBMP(3, 25, 10, 14, image_BAT_percent_bits);

    u8g2.drawXBMP(4, 47, 35, 9, image_CPU_bar_bits);

    int barWidth = cpu_usage * 31 / 100;
    u8g2.drawBox(6, 49, barWidth, 5);

    // RAM
    u8g2.drawStr(51, 16, "RAM");

    char ramUsedStr[16];
    snprintf(ramUsedStr, sizeof(ramUsedStr), "%.1f", used_memory);
    u8g2.drawStr(46, 37, ramUsedStr);

    u8g2.drawStr(75, 37, "/");

    char ramTotalStr[16];
    snprintf(ramTotalStr, sizeof(ramTotalStr), "%d", total_memory);
    u8g2.drawStr(46, 55, ramTotalStr);

    u8g2.drawStr(65, 55, "GB");

    // Uptime
    u8g2.drawStr(88, 16, "UP");

    char daysStr[12];
    snprintf(daysStr, sizeof(daysStr), "%luD", uptime_days);
    u8g2.drawStr(90, 35, daysStr);

    char hoursStr[12];
    snprintf(hoursStr, sizeof(hoursStr), "%luH", uptime_hours);
    u8g2.drawStr(90, 55, hoursStr);
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
    Serial.setTimeout(20);

    u8g2.begin();
}

void loop()
{
    if (Serial.available())
    {
        size_t len = Serial.readBytesUntil(
            '\n',
            serialBuffer,
            sizeof(serialBuffer) - 1);

        serialBuffer[len] = '\0';

        parseData(serialBuffer);
    }

    u8g2.firstPage();
    do
    {
        drawScreen();
    }
    while (u8g2.nextPage());

    delay(50);
}
