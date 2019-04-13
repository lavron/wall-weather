#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Time.h>

#define TIME_OFFSET 3UL * 3600UL // UTC + 3 hour

#define SSID "iot"
#define SSID_PASSWORD "Deadbobr23!"

#define ENABLE_GxEPD2_GFX 0
#define GxEPD_YELLOW       0xFFE0
#include "GxEPD2_boards_added.h"
#include <GxEPD2_3C.h>

//n-now, d-day, t-tomorrow
int temp_n,
    temp_d_max,
    temp_d_min,
    temp_t_max,
    temp_t_min,
    rain_d, rain_t;

String icon_n,
       icon_d,
       icon_t,
       last_update_time,
       message;


unsigned long previousMillis = 0;
//const long interval = 30e3; ///1000s~15m
const long interval = 1000e3; ///1000s~15m

const char* fingerprint = "07 5C DA 63 8E 13 BB 68 F1 4C 20 AB 4B 27 45 5C C9 A9 12 96"; //https://lavron.info


GxEPD2_3C < GxEPD2_750c, GxEPD2_750c::HEIGHT / 4 > display(GxEPD2_750c(/*CS=15*/ SS, /*DC=4*/ 4, /*RST=5*/ 5, /*BUSY=16*/ 16));
ESP8266WiFiMulti WiFiMulti;

#include "OpenSans.h"
#include <Fonts/FreeMono9pt7b.h>
#include "icons.h"

bool REFRESH_NEED = false;


void setup() {


  Serial.begin(115200);
  while (!Serial) {
    ;
  }

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(SSID, SSID_PASSWORD);
  display.init(115200);
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval || previousMillis == 0) {
    previousMillis = currentMillis;
    get_weather();
  }
  if (REFRESH_NEED) {
    refresh_display();
  }
}

void get_weather() {
  Serial.println("get_weather");

  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }


  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);

  client->setFingerprint(fingerprint);

  HTTPClient https;

  Serial.print("[HTTPS] begin...\n");

  while (!https.begin(*client, "https://lavron.info/api/weather/")) {
    delay(500);
    Serial.print("~");
  }


  Serial.print("[HTTPS] GET...\n");
  int httpCode = https.GET();

  if (!(httpCode > 0)) {
    Serial.printf("[HTTPS] Unable to connect\n");
    return;
  }

  Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

  // file found at server
  if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
    String payload = https.getString();
    Serial.println(payload);

    DynamicJsonDocument jsonDocument(500);


    DeserializationError error = deserializeJson(jsonDocument, payload);
    if (error) {
      Serial.println("There was an error while deserializing");
      Serial.println(error.c_str());
    }  else {

      JsonObject root = jsonDocument.as<JsonObject>();

      const char* icon_temp;
      const char* last_update_time_temp;

      String weather_summary_new, payload_new;

      temp_n = root["message"]["now"]["temp"];
      temp_d_max = root["message"]["today"]["temp_max"];
      temp_d_min = root["message"]["today"]["temp_min"];
      temp_t_max = root["message"]["tomorrow"]["temp_max"];
      temp_t_min = root["message"]["tomorrow"]["temp_min"];

      rain_d = root["message"]["today"]["rain"];
      rain_t = root["message"]["tomorrow"]["rain"];


      icon_temp = root["message"]["now"]["icon"];
      icon_n = icon_temp;
      icon_temp = root["message"]["today"]["icon"];
      icon_d = icon_temp;
      icon_temp = root["message"]["tomorrow"]["icon"];
      icon_t = icon_temp;

      last_update_time_temp = root["message"]["time"];
      last_update_time = last_update_time_temp;

      if (payload_new == payload) {
        Serial.println("no REFRESH_NEED");
      } else {
        Serial.println("REFRESH_NEED");
        REFRESH_NEED = true;
        payload = payload_new;
      }

      Serial.print("temp:"); Serial.println(temp_n);

    }
  } else {
    Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
  }

  https.end();
}

void refresh_display() {
  Serial.println("refresh_display");
  int16_t  x1, y1, x, y;
  uint16_t w, h;

  display.setFullWindow();
  display.fillScreen(GxEPD_WHITE);
  display.setRotation(0);
  display.firstPage();


  //randomizing screen layout

  int rnd_x_temp = random(30);
  int rnd_y_temp = random(50);
  int rnd_x_icon = random(200);
  int rnd_y_icon = random(50);
  int rnd_x_footer = random(400);


  display.firstPage();
  do {
    print_current_temp (rnd_x_temp, rnd_y_temp);
    print_icon(300 + rnd_x_icon, 70 + rnd_y_icon);

    print_footer(rnd_x_footer, 370, temp_d_max, temp_d_min, rain_d);
    print_footer(rnd_x_footer + 300, 370, temp_t_max, temp_t_min, rain_t);

    print_sync_tyme(576, 382);
  }  while (display.nextPage());

  Serial.println("refresh_display done");
  display.powerOff();

  REFRESH_NEED = false;
}

void print_footer(int16_t x, int16_t y, int temp_max,  int temp_min, int rain) {
  //  int16_t  x1, y1;
  //  uint16_t w, h;

  display.setFont(&Open_Sans_Regular_36);

  display.setCursor(x, y);

  display.setTextColor(GxEPD_YELLOW);
  display.print(temp_max);
  display.setTextColor(GxEPD_BLACK);
  display.print(" | ");
  display.print(temp_min);

  //rain icon
  if (rain > 0 ) {
    display.setTextColor(GxEPD_BLACK);
    
    //trying to get width of the block with text in order to get an icon offset
    //    String all_text = temp_d_max + " | " + temp_d_min;
    //    display.getTextBounds(all_text, 0, 0, &x1, &y1, &w, &h);
    //    x = x + w;

    //no luck, just add 130 px offset
    x = x + 130;
    display.drawBitmap( x, y - 24, rain_sm, 24, 24, GxEPD_BLACK);
    x = x + 24 + 10;
    display.setCursor(x, y);
    display.print(rain);
    display.print("%");
  }

}


void print_sync_tyme( int16_t x, int16_t y) {
  display.setFont(&FreeMono9pt7b);
  display.setTextColor(GxEPD_YELLOW);
  display.setCursor(x, y);
  display.print(last_update_time);
}


void print_current_temp( int16_t x, int16_t y) {
  int16_t  x1, y1;
  uint16_t w, h;
  display.setFont(&Open_Sans_Regular_288);
  display.setTextColor(GxEPD_YELLOW);
  display.setCursor(x, y);
  display.print(temp_n);
  //degree icon
  display.getTextBounds(String(temp_n), x, y, &x1, &y1, &w, &h);
  x = x + w + 20; y = y + 40;
  display.drawBitmap( x, y, degree, 83, 82, GxEPD_BLACK);
}

void print_icon( int16_t x, int16_t y) {
  //sun
  if  (icon_n == "clear-day") {
    display.drawBitmap( x, y, sun, 160, 160, GxEPD_YELLOW);

    //moon
  } else if (icon_n == "clear-night") {
    display.drawBitmap( x, y, moon, 160, 176, GxEPD_BLACK);

    //cloud-sun
  } else if (icon_n == "partly-cloudy-day") {
    display.drawBitmap( x, y, cloud_sun_y, 121, 122, GxEPD_YELLOW);
    display.drawBitmap( x + 55, y + 62, cloud_sun_g, 145, 98, GxEPD_BLACK);
    //    display.drawBitmap( x + 55, y + 62, cloud_sun_b, 145, 98, GxEPD_BLACK);

    //cloud-moon
  } else if (icon_n == "partly-cloudy-night") {
    display.drawBitmap( x, y, cloud_moon, 180, 160, GxEPD_BLACK);

    //cloud
  } else if (icon_n == "cloudy" || icon_n == "fog") {
    display.drawBitmap( x, y, cloud_g, 200, 140, GxEPD_BLACK);

    //rain
  } else if (icon_n == "rain" ) {
    display.drawBitmap( x, y, rain, 160, 160, GxEPD_BLACK);

    //storm //NOT in use with Darksky
  } else if (icon_n == "storm" ) {
    display.drawBitmap( x, y, storm_b, 175, 150, GxEPD_BLACK);
    display.drawBitmap( x + 62, y + 100, storm_y, 63, 100, GxEPD_YELLOW);

    //snow
  } else if (icon_n == "snow" || icon_n == "sleet" ) {
    display.drawBitmap( x, y, snow, 176, 200, GxEPD_BLACK);

  } else {
    display.drawBitmap( x, y, storm_b, 175, 150, GxEPD_BLACK);
    display.drawBitmap( x + 62, y + 100, storm_y, 63, 100, GxEPD_YELLOW);
  }


}
