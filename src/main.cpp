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

// bitmaps for icons
static const unsigned char image_BAT_percent_bits[] U8X8_PROGMEM = {0x0c,0x03,0x1e,0x03,0x33,0x03,0xb3,0x03,0xde,0x01,0xec,0x00,0x70,0x00,0x38,0x00,0xdc,0x00,0xee,0x01,0x37,0x03,0x33,0x03,0xe3,0x01,0xc3,0x00};
static const unsigned char image_battery_charging_bits[] U8X8_PROGMEM = {0x00,0x40,0x00,0xf0,0x27,0x7f,0x08,0x30,0x80,0x08,0x10,0x80,0x0e,0x18,0x80,0x01,0x0c,0x80,0x01,0xfc,0x81,0x01,0xfe,0x80,0x01,0xc0,0x80,0x01,0x60,0x80,0x0e,0x20,0x80,0x08,0x30,0x80,0x08,0x10,0x80,0xf0,0xcb,0x7f,0x00,0x08,0x00,0x00,0x00,0x00};
static const unsigned char image_CPU_bar_bits[] U8X8_PROGMEM = {0xfc,0xff,0xff,0xff,0x01,0x02,0x00,0x00,0x00,0x02,0x01,0x00,0x00,0x00,0x04,0x01,0x00,0x00,0x00,0x04,0x01,0x00,0x00,0x00,0x04,0x01,0x00,0x00,0x00,0x04,0x01,0x00,0x00,0x00,0x04,0x02,0x00,0x00,0x00,0x02,0xfc,0xff,0xff,0xff,0x01};

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
    // RAM title
    u8g2.setFont(u8g2_font_t0_17b_tr);
    u8g2.drawStr(51, 16, "RAM");
    // RAM used
    char ram_used_str[10];
    sprintf(ram_used_str, "%.1f", used_memory);
    u8g2.drawStr(46, 37, ram_used_str);
    // RAM seperator
    u8g2.drawStr(75, 37, "/");
    // RAM total
    char ram_total_str[10];
    sprintf(ram_total_str, "%d", total_memory);
    u8g2.drawStr(46, 55, ram_total_str);
    // RAM total gb
    u8g2.drawStr(65, 55, "GB");
    // BAT title
    u8g2.setFont(u8g2_font_t0_17b_tr);
    u8g2.drawStr(94, 16, "BAT");
    // BAT charge
    char battery_charge_str[10];
    sprintf(battery_charge_str, "%d", battery_charge);
    u8g2.drawStr(103, 38, battery_charge_str);
    // BAT percent
    u8g2.drawXBMP(91, 25, 10, 14, image_BAT_percent_bits);
    // battery_charging
    if (is_plugged_in) {
      u8g2.drawXBMP(96, 45, 24, 16, image_battery_charging_bits);
    }
    // CPU title
    u8g2.drawStr(7, 16, "CPU");
    // CPU load
    char cpu_usage_str[10];
    sprintf(cpu_usage_str, "%d", cpu_usage);
    u8g2.drawStr(15, 38, cpu_usage_str);
    // CPU percent
    u8g2.drawXBMP(3, 25, 10, 14, image_BAT_percent_bits);
    // CPU bar
    u8g2.drawXBMP(4, 47, 35, 9, image_CPU_bar_bits);
    // CPU bar filling
    u8g2.drawBox(6, 49, cpu_usage * 31 / 100, 5);
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
