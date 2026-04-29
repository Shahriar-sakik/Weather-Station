#include <WiFi.h>
#include <HTTPClient.h>
#include "DHTesp.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

const char* WIFI_NAME = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

String apiKey = "NNE4TPHZG12J8IDB";

const int DHT_PIN = 15;

DHTesp dhtSensor;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

unsigned long lastThingSpeakTime = 0;
unsigned long lastScreenChange = 0;

const unsigned long thingSpeakDelay = 20000;
const unsigned long screenDelay = 3000;

int screenMode = 0;

void setup() {
  Serial.begin(115200);

  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED failed");
    while (true);
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Weather Station");
  display.println("Starting...");
  display.display();

  WiFi.begin(WIFI_NAME, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("WiFi Connected");
  display.display();

  delay(1500);
}

void drawHourlyTemp() {
  display.setTextSize(1);

  // 2h forecast
  display.setCursor(0, 44);
  display.print("2h");
  display.setCursor(0, 56);
  display.print("22C");

  // Rain icon
  display.drawLine(18, 44, 24, 44, WHITE);
  display.drawLine(16, 46, 26, 46, WHITE);
  display.drawPixel(18, 50, WHITE);
  display.drawPixel(22, 52, WHITE);

  // 4h forecast
  display.setCursor(42, 44);
  display.print("4h");
  display.setCursor(42, 56);
  display.print("24C");

  // Cloud icon
  display.drawCircle(62, 46, 3, WHITE);
  display.drawCircle(67, 46, 3, WHITE);
  display.drawLine(59, 49, 70, 49, WHITE);

  // 6h forecast
  display.setCursor(84, 44);
  display.print("6h");
  display.setCursor(84, 56);
  display.print("25C");

  // Sun icon
  display.drawCircle(108, 47, 3, WHITE);
  display.drawLine(108, 41, 108, 43, WHITE);
  display.drawLine(108, 51, 108, 53, WHITE);
  display.drawLine(102, 47, 104, 47, WHITE);
  display.drawLine(112, 47, 114, 47, WHITE);
}

void loop() {
  TempAndHumidity data = dhtSensor.getTempAndHumidity();

  float temp = data.temperature;
  float hum = data.humidity;

  if (isnan(temp) || isnan(hum)) {
    Serial.println("DHT22 read failed");

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Sensor Error");
    display.println("Check DHT22");
    display.display();

    delay(2000);
    return;
  }

  String status;
  if (temp < 10) {
    status = "Cold";
  } else if (temp <= 25) {
    status = "OK";
  } else {
    status = "Hot";
  }

  String feelsLike;
  if (temp > 25 || hum > 65) {
    feelsLike = "Hot";
  } else if (temp < 10) {
    feelsLike = "Cold";
  } else {
    feelsLike = "Normal";
  }

  if (millis() - lastScreenChange >= screenDelay) {
    screenMode++;

    if (screenMode > 2) {
      screenMode = 0;
    }

    lastScreenChange = millis();
  }

  display.clearDisplay();
  display.setTextColor(WHITE);

  // Top line
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Lahti");

  display.setCursor(105, 0);
  display.print(")))");

  // Temperature
  display.setTextSize(2);
  display.setCursor(0, 14);
  display.print(temp, 1);
  display.print(" C");

  // Humidity
  display.setTextSize(1);
  display.setCursor(96, 18);
  display.print("Hum");

  display.setCursor(96, 31);
  display.print(hum, 0);
  display.print("%");

  // Rotating bottom section
  if (screenMode == 0) {
    display.setCursor(0, 52);
    display.print("Status: ");
    display.print(status);
  }

  if (screenMode == 1) {
    drawHourlyTemp();
  }

  if (screenMode == 2) {
    display.setCursor(0, 52);
    display.print("Feels like:");
    display.setCursor(70, 52);
    display.print(feelsLike);
  }

  display.display();

  Serial.println("Temp: " + String(temp));
  Serial.println("Humidity: " + String(hum));
  Serial.println("Status: " + status);
  Serial.println("Feels like: " + feelsLike);

  if (WiFi.status() == WL_CONNECTED &&
      millis() - lastThingSpeakTime >= thingSpeakDelay) {

    HTTPClient http;

    String url = "http://api.thingspeak.com/update?api_key=" +
                 apiKey +
                 "&field1=" + String(temp, 2) +
                 "&field2=" + String(hum, 1);

    http.begin(url);
    int responseCode = http.GET();

    if (responseCode > 0) {
      Serial.println("Data sent to ThingSpeak");
    } else {
      Serial.println("Error sending data");
    }

    http.end();
    lastThingSpeakTime = millis();
  }

  delay(2000);
}