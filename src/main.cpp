#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <Adafruit_SHT31.h>
#include <Adafruit_BMP085.h>
#include <stdint.h>
#define U8G2_FONT_SECTION(name)
#include "u8g2_font_FreeSans.h"

// 1.3inch, 240x240
#if defined(ESP32)
  #define DISP_SCL 18
  #define DISP_SDA 23
  #define DISP_RES 26
  #define DISP_DC  17
  #define DISP_CS  -1
  #define DISP_BLK 5
  #include "LGFX_ESP32_ST7789_SPI.hpp"
  LGFX_ESP32_ST7789_SPI display(240, 240, DISP_SCL, DISP_SDA, DISP_RES, DISP_DC, DISP_CS, DISP_BLK, -1, -1);
#elif defined(ESP8266)
  #define DISP_SCL 14
  #define DISP_SDA 13
  #define DISP_RES 16
  #define DISP_DC  0
  #define DISP_CS  -1
  #define DISP_BLK 15
  #include "LGFX_ESP8266_ST7789_SPI.hpp"
  LGFX_ESP8266_ST7789_SPI display(240, 240, DISP_SCL, DISP_SDA, DISP_RES, DISP_DC, DISP_CS, DISP_BLK, -1, -1);
#endif
LGFX_Sprite canvas(&display);

static const lgfx::U8g2font u8g2font40(u8g2_font_FreeSansBoldOblique40pt7b);
static const lgfx::U8g2font u8g2font18a(u8g2_font_FreeSansBoldOblique18pt7ba);

Adafruit_BMP085 bmp180;
Adafruit_SHT31 sht31 = Adafruit_SHT31();

void DrawParameter(int line, float val, int format, String drawunit, int warn_col);

void setup()
{
  Serial.begin(115200);

  if (!bmp180.begin()) {
    Serial.println("Couldn't find BMP180");
    while (1) delay(1);
  }

  if (!sht31.begin(0x44)) {
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }

  display.init();
  display.setRotation(0);
#if defined(ESP32)
  // display.setBrightness(255); // 100%
  display.setBrightness(64);  // 25%
#elif defined(ESP8266)
  pinMode(DISP_BLK, OUTPUT);
  // analogWrite(DISP_BLK, 255); // 100%
  analogWrite(DISP_BLK, 64);  // 25%
#endif
  display.setColorDepth(4);
  display.fillScreen(TFT_BLACK);
  canvas.setColorDepth(4);  // 4ビット(16色)パレットモード
  canvas.createSprite(240, 240);
  canvas.setPaletteColor(1, 255,   0,   0); // RED
  canvas.setPaletteColor(2,   0, 255,   0); // GREEN
  canvas.setPaletteColor(3,   0,   0, 255); // BLUE
  canvas.setPaletteColor(4, 255, 255,   0); // YELLOW
// カラー定義を16色パレットモードに割り付け直す
#undef TFT_RED
#define TFT_RED     1
#undef TFT_GREEN
#define TFT_GREEN   2
#undef TFT_BLUE
#define TFT_BLUE    3
#undef TFT_YELLOW
#define TFT_YELLOW  4
}

void loop() {
  static bool working_dot = false;
  int warn_col = TFT_WHITE; // 警告表示文字色

  // センサー値取得
  float T = bmp180.readTemperature();           // BMP180: 温度
  float P = (float)bmp180.readPressure() / 100; // BMP180: 気圧
  float t = sht31.readTemperature();            // SHT31:  温度
  float h = sht31.readHumidity();               // SHT31:  湿度

  // 標準計測器？（SwitchBot温湿度計）を正として簡易補正
  T *= 0.92;
  t *= 0.91;
  h *= 1.09;

  canvas.clear();

  // BMP180: 温度
  // 室内で快適に過ごせる気温は夏場で25～28度、冬場は18～22度程度と言われている
  warn_col = TFT_WHITE;
  if ((T < 18) || (T > 28))  warn_col = TFT_YELLOW;
  if ((T < 17) || (T > 29))  warn_col = TFT_RED;
  DrawParameter(0, T, 1, "℃", warn_col);

  // BMP180: 気圧
  // 天気痛が分かる値としたかったが単純に現在値だけでは分からないので、なんとなく値を入れてみた
  warn_col = TFT_WHITE;
  if (P <= 1003)  warn_col = TFT_YELLOW;
  if (P < 1000)  warn_col = TFT_RED;
  DrawParameter(1, P, 0, "hPa", warn_col);

  // SHT31: 温度
  warn_col = TFT_WHITE;
  if ((t < 18) || (t > 28))  warn_col = TFT_YELLOW;
  if ((t < 17) || (t > 29))  warn_col = TFT_RED;
  DrawParameter(2, t, 1, "℃", warn_col);

  // SHT31: 湿度
  // 室内で快適に過ごせる湿度は夏場で45～60%、冬場は55～65%程度と言われている
  warn_col = TFT_WHITE;
  if ((h < 45) || (h > 65))  warn_col = TFT_YELLOW;
  if ((h < 40) || (h > 70))  warn_col = TFT_RED;
  DrawParameter(3, h, 0, "%", warn_col);

  working_dot = !working_dot;
  if (working_dot == true) {
    canvas.fillCircle(10, 180 + 50, 5, TFT_WHITE);
  }

  canvas.pushSprite(0, 0);

  delay(1000);
}

void DrawParameter(int line, float val, int format, String drawunit, int warn_col)
{
  String Str_set;
  char str[8];
  int num_size;   // 数字フォント基準幅（フォント毎に固定：調整不可）
  int dot_size;   // 小数点フォント幅（フォント毎に固定：調整不可）
  int x_adj;      // x軸右寄せ調整値（フォント毎に固定：調整不可）

  int rx = 170;         // 右寄せ位置
  int line_space = 60;  // 行間隔
  int x_offset = 6;     // X軸オフセット
  int y_offset = 4;     // Y軸オフセット

  if (format == 0) {
    Str_set = String((int)val);
  } else {
    Str_set = String(val, format);
  }
  Str_set.toCharArray(str, sizeof(str));

  canvas.setTextSize(1);
  canvas.setTextColor(warn_col);
  canvas.setFont(&u8g2font40);
  canvas.setTextDatum(textdatum_t::top_left);
  canvas.setFont(&u8g2font40); num_size = 40;  dot_size = 20;  x_adj = 12;
  canvas.drawString(str, rx - (strlen(str) * num_size) - (format ? 0 : dot_size) + x_adj + x_offset, (line * line_space) + y_offset);
  if (drawunit == "℃") {
    canvas.setFont(&u8g2font18a);
  } else {
    canvas.setFont(&fonts::FreeSansBoldOblique18pt7b);
  }
  canvas.setTextDatum(textdatum_t::bottom_left);
  canvas.drawString(drawunit, rx, ((line + 1) * line_space));
}
