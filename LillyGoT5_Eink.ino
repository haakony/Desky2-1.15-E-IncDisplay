
//https://github.com/Xinyuan-LilyGO/LilyGo-T5-Epaper-Series
// include library, include base class, make path known
// Link for downloading GxEPD library :- https://github.com/ZinggJM/GxEPD
//#include <GxEPD.h>
#include <GxEPD2_BW.h> // including both doesn't use more code or ram
#include <GxEPD2_3C.h> // including both doesn't use more code or ram

#include "Adafruit_GFX.h" //https://learn.adafruit.com/adafruit-gfx-graphics-library?view=all

//#include <GxEPD_BitmapExamples>

// FreeFonts from Adafruit_GFX
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>

//#include <GxIO/GxIO_SPI/GxIO_SPI.h>
//#include <GxIO/GxIO.h>
#include <WiFi.h>
#include <WiFiMulti.h>

#include <HTTPClient.h>
#include <ArduinoJson.h>

//#include "env.h"
#include "bitmap.h"
#include "helpers.h"

//****************************** Screensshit

// Color definitions
#define BLACK 0x0000
#define WHITE 0xFFFF
String line1Data;
String stockLine1 = "NTEL";
String stockLine2;
String stockLine3;
String btcLine1 = "BTC";
String btcLine2;
String btcLine3;
String ytLine1 = "YT";
String ytLine2;
String ytLine3;
bool onpower = true;
int screenNr = 1;
int screenNrs = 4;
bool firstLoop = true;
int loopNr = 1;

//btn
const int PushButton = 39;

//volts
const int ADC_PIN = 35;
int rawVoltValue;
float voltage;
float voltPerc;

//timer
unsigned long updateScrn = 0;
unsigned long lastupdateScrn = 0;
int screenUpdateTime = 3600000; //how often to update screen when on power
int screenUpdateTime2;
int rollingScreenTime = 300000;

WiFiMulti WiFiMulti;

#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 3600     /* Time ESP32 will go to sleep (in seconds) */

RTC_DATA_ATTR int bootCount = 0;

// for SPI pin definitions see e.g.:
// C:\Users\xxx\Documents\Arduino\hardware\espressif\esp32\variants\lolin32\pins_arduino.h

//GxIO_Class io(SPI, /*CS=5*/ SS, /*DC=*/ 17, /*RST=*/ 16); // arbitrary selection of 17, 16
GxEPD2_BW<GxEPD2_213_B74, GxEPD2_213_B74::HEIGHT> display(GxEPD2_213_B74(/*CS=5*/ SS, /*DC=*/17, /*RST=*/16, /*BUSY=*/4)); // GDEM0213B74

//setup
void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println("setup");

  display.init(115200); // enable diagnostic output on Serial
  pinMode(PushButton, INPUT);
  Serial.println("setup done");

  for (uint8_t t = 4; t > 0; t--)
  {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }
  print_wakeup_reason();
  screenUpdateTime2 = screenUpdateTime;
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("name", "pass");

  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));


  /*
    First we configure the wake up source
    We set our ESP32 to wake up every x seconds
  */
  touch_pad_intr_disable(); //deepsleep tuch disable
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
                 " Seconds");
}

void loop()
{
  if (firstLoop)
  {
    batteryIndicator();
    runWifiAndGetJSON();
    screenchoice();
    firstLoop = false;
  }

  if (millis() - lastupdateScrn > screenUpdateTime)
  {
    lastupdateScrn = millis();
    //do somthing
    batteryIndicator();
    runWifiAndGetJSON();
    screenchoice();
    updateScrn++;
  }

  int Push_button_state = digitalRead(PushButton);
  // if condition checks if push button is pressed
  // if pressed LED will turn on otherwise remain off
  if (Push_button_state == HIGH)
  {
    //Serial.println("btn high");
  }
  else
  {
    Serial.println("btn low");
    if (screenNr == 1)
    {
      screenNr = 2;
    }
    else if (screenNr == 2)
    {
      screenNr = 3;
    }
    else if (screenNr == 3)
    {
      screenNr = 4;
    }
    else if (screenNr == 4)
    {
      screenNr = 1;
    }
    Serial.println("srn=");
    Serial.println(screenNr);
    screenchoice();
  }


  //sleep cycle
  //delay(500);
  if (voltPerc < float(50))
  {
    onpower = false;
    screenchoice();
    // It will go into deep Sleep for 60 sec. if less then 2080battery
    Serial.println("Going to sleep now " + String(voltPerc) + "power");
    delay(100);
    esp_deep_sleep_start();
  }
  else
  {
    onpower = true;
  }

} //end update





void screenchoice()
{
  if (screenNr == 1)
  {
    stockScreen();
  }
  if (screenNr == 2)
  {
    CryptoScreen();
  }
  if (screenNr == 3)
  {
    YouTubeScreen();
  }

  if (screenNr == 4)
  {
    screenUpdateTime = rollingScreenTime;
    if (loopNr == 1)
    {
      stockScreen();
      loopNr++;
    }
    else if (loopNr == 2)
    {
      CryptoScreen();
      loopNr++;
    }
    else if (loopNr == 3)
    {
      YouTubeScreen();
      loopNr = 1;
    }
    Serial.println("Loop nr " + loopNr);
  }
  else
  {
    screenUpdateTime = screenUpdateTime2;
  }
}

void CryptoScreen()
{
  // Setting the characters of font to be displayed on the board.
  const char *name = "FreeMonoBold12pt7b";
  const GFXfont *f = &FreeMonoBold12pt7b;
  const GFXfont *f2 = &FreeMonoBold9pt7b;
  const GFXfont *f18 = &FreeMonoBold18pt7b;
  const GFXfont *f3 = &FreeMonoBold24pt7b;

  display.setRotation(1);
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  uint16_t x = 2;  //display.width() / 2 -60; // horizen
  uint16_t y = 20; //display.height() / 2;   // Vertical

  //Printing all the data on the display

  display.drawBitmap(x, y - 20, epd_bitmap_bitcoin_PNG48, 64, 64, BLACK);
  //display.setCursor(x, y + 30);
  display.setCursor(x + 64, y);
  display.setFont(f18);
  display.print("BitCoin");
  //display.setCursor(x+64 , y-30);
  display.println();
  display.setCursor(x + 64, y + 32);
  display.print(btcLine1);
  display.setFont(f);
  display.println();
  display.print(btcLine2);
  display.println();
  display.print(btcLine3);

  //dataline / battery
  display.setFont();
  display.setCursor(x, y + 100);
  display.print(line1Data);
  display.print(" V");
  display.print(screenNr);
  display.setCursor(x + 200, y + 100);
  //display.print(String(voltage));

  display.print("Bat:");
  display.print(String(int(voltPerc)));

  display.nextPage();
}

void stockScreen()
{

  // Setting the characters of font to be displayed on the board.
  const char *name = "FreeMonoBold12pt7b";
  const GFXfont *f = &FreeMonoBold12pt7b;
  const GFXfont *f2 = &FreeMonoBold9pt7b;
  const GFXfont *f18 = &FreeMonoBold18pt7b;
  const GFXfont *f3 = &FreeMonoBold24pt7b;

  display.setRotation(1);
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  uint16_t x = 2;  //display.width() / 2 -60; // horizen
  uint16_t y = 20; //display.height() / 2;   // Vertical
  // drawBitmap(x position, y position, bitmap data, bitmap width, bitmap height, color)
  display.drawBitmap(x, y - 20, epd_bitmap_nortelo, 64, 64, BLACK);
  //display.setCursor(x, y + 30);
  display.setCursor(x + 64, y);
  display.setFont(f18);
  display.print("Nortel");
  //display.setCursor(x+64 , y-30);
  display.println();
  display.setCursor(x + 64, y + 32);
  display.print(stockLine1);
  display.setFont(f);
  display.println();
  display.print(stockLine2);
  display.println();
  display.print(stockLine3);

  //dataline / battery
  display.setFont();
  display.setCursor(x, y + 100);
  display.print(line1Data);
  display.print(" V");
  display.print(screenNr);
  display.setCursor(x + 200, y + 100);
  //display.print(String(voltage));

  display.print("Bat:");
  display.print(String(int(voltPerc)));

  display.nextPage();
}

void YouTubeScreen()
{

  // Setting the characters of font to be displayed on the board.
  const char *name = "FreeMonoBold12pt7b";
  const GFXfont *f = &FreeMonoBold12pt7b;
  const GFXfont *f2 = &FreeMonoBold9pt7b;
  const GFXfont *f18 = &FreeMonoBold18pt7b;
  const GFXfont *f3 = &FreeMonoBold24pt7b;

  display.setRotation(1);
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  uint16_t x = 2;  //display.width() / 2 -60; // horizen
  uint16_t y = 20; //display.height() / 2;   // Vertical
  display.drawBitmap(x, y - 20, epd_bitmap_youtube, 64, 64, BLACK);
  //display.setCursor(x, y + 30);
  display.setCursor(x + 64, y);
  display.setFont(f18);
  display.print("YouTube");
  //display.setCursor(x+64 , y-30);
  display.println();
  display.setCursor(x + 64, y + 32);
  display.print(ytLine1);
  display.setFont(f);
  display.println();
  display.print(ytLine2);
  display.println();
  display.print(ytLine3);

  //dataline / battery
  display.setFont();
  display.setCursor(x, y + 100);
  display.print(line1Data);
  display.print(" V");
  display.print(screenNr);
  display.setCursor(x + 200, y + 100);
  //display.print(String(voltage));

  display.print("Bat:");
  display.print(String(int(voltPerc)));

  display.nextPage();
}

void batteryIndicator()
{

  //batterycalk
  rawVoltValue = analogRead(ADC_PIN);
  voltage = (rawVoltValue / 4095.0) * 7.26;
  voltPerc = map(voltage, 3.6, 4.2, 0, 100);
}

void runWifiAndGetJSON()
{
  if ((WiFiMulti.run() == WL_CONNECTED))
  {

    HTTPClient http;

    Serial.print("[HTTP] begin...\n");
    // configure traged server and url
    //http.begin("http://worldclockapi.com/api/json/utc/now"); //HTTP
    http.begin("http://bot.haakony.no/desky");

    Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();
    //String payload = http.getString();
    //Serial.println(payload);

    // httpCode will be negative on error
    if (httpCode > 0)
    {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK)
      {

        // Parse response
        StaticJsonDocument<1024> doc;
        DeserializationError err = deserializeJson(doc, http.getStream());
        if (err)
        {
          Serial.print(F("deserializeJson() failed with code "));
          Serial.println(err.f_str());
        }

        //Serial.println(const char* name = doc["name"];);
        line1Data = doc["data"]["line1"].as<String>();

        stockLine1 = doc["stock"]["line1"].as<String>();
        stockLine2 = doc["stock"]["line2"].as<String>();
        stockLine3 = doc["stock"]["line3"].as<String>();

        btcLine1 = doc["btc"]["line1"].as<String>();
        btcLine2 = doc["btc"]["line2"].as<String>();
        btcLine3 = doc["btc"]["line3"].as<String>();

        ytLine1 = doc["youtube"]["line1"].as<String>();
        ytLine2 = doc["youtube"]["line2"].as<String>();
        ytLine3 = doc["youtube"]["line3"].as<String>();

        Serial.println(line1Data);
        Serial.println(stockLine1);
        Serial.println(stockLine1);
      }
    }
    else
    {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  }
}
