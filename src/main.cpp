#include <Arduino.h>
#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

// SSD1306 128x64 noname I2C setup
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// bitmap for battery icon
static const unsigned char image_Battery_bits[] U8X8_PROGMEM = {0xfc,0x03,0x12,0x04,0x53,0x05,0x53,0x05,0x42,0x04,0xfc,0x03};

// variables from mac
int cpu_usage = 0;
float used_memory = 0.0;
int total_memory = 0;
int battery_charge = 0;
bool is_plugged_in = false;

// funciton for lopaka screen
void drawScreen_1(void) {
    u8g2.setFontMode(1);
    u8g2.setBitmapMode(1);
    // string 1
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.drawStr(4, 13, "CPU:");
    // cpu bar
    u8g2.drawFrame(23, 6, 77, 9);
    // cpu usage fill
    u8g2.drawBox(24, 7, cpu_usage * 75 / 100, 7);
    // string 3
    u8g2.drawStr(103, 13, "%");
    // string 4
    u8g2.drawStr(4, 35, "RAM Usage: ");
    // used ram
    char ram_used_str[10];
    sprintf(ram_used_str, "%.1f", used_memory);
    u8g2.drawStr(48, 35, ram_used_str);
    // string 6
    u8g2.drawStr(64, 35, "/");
    // total ram
    char ram_total_str[10];
    sprintf(ram_total_str, "%d", total_memory);
    u8g2.drawStr(72, 35, ram_total_str);
    // string 8
    u8g2.drawStr(82, 35, "GB");
    // string 9
    u8g2.drawStr(4, 55, "Battery:");
    // cpu usage percentage
    char cpu_str[4];
    sprintf(cpu_str, "%d", cpu_usage);
    u8g2.drawStr(108, 13, cpu_str);
    // battery percentage
    char battery_str[4];
    sprintf(battery_str, "%d", battery_charge);
    u8g2.drawStr(38, 55, battery_str);
    // string 12
    u8g2.drawStr(51, 55, "%");
    // plugged in status
    if (is_plugged_in) {
      u8g2.drawXBMP(60, 49, 11, 6, image_Battery_bits);
    }
}

void setup() {
  Serial.begin(115200);
  u8g2.begin();
}

void loop() {
  if (Serial.available() > 0) {
    String data = Serial.readStringUntil('\n');
    // split data into parts
    int cpu_index = data.indexOf("CPU:");
    int ram_used_index = data.indexOf("RAM_USED:");
    int ram_total_index = data.indexOf("RAM_TOTAL:");
    int battery_index = data.indexOf("BATTERY:");
    int plugged_index = data.indexOf("PLUGGED:");

    if (cpu_index != -1 && ram_used_index != -1 && ram_total_index != -1 && battery_index != -1 && plugged_index != -1) {
      String cpu_str = data.substring(cpu_index + 4, ram_used_index);
      String ram_used_str = data.substring(ram_used_index + 9, ram_total_index);
      String ram_total_str = data.substring(ram_total_index + 10, battery_index);
      String battery_str = data.substring(battery_index + 8, plugged_index);
      String plugged_str = data.substring(plugged_index + 8);

      cpu_usage = cpu_str.toInt();
      used_memory = ram_used_str.toFloat();
      total_memory = ram_total_str.toInt();
      battery_charge = battery_str.toInt();
      is_plugged_in = (plugged_str == "True") ? true : false;
    }
  }

  u8g2.firstPage();
  do {
    drawScreen_1();
  } while (u8g2.nextPage());

  delay(50);
}
