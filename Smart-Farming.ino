#define BLYNK_TEMPLATE_ID "TMPL6jMKB9ZtT"
#define BLYNK_TEMPLATE_NAME "Jihad Project"
#define BLYNK_AUTH_TOKEN "A0QSGC0AfgHjB39hHO6bkTs91iNBlljh"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// === WiFi Credentials ===
char ssid[] = "Jihad";
char pass[] = "12345678";

// === Sensor Pins ===
#define DHTPIN 14
#define DHTTYPE DHT11
#define SOIL_MOISTURE_PIN 34
#define PH_SENSOR_PIN 35
#define WATER_LEVEL_PIN 36  // VP pin
#define BUZZER_PIN 25

// === Output Pins ===
#define RELAY_PIN 26      // Water Pump Relay
#define FAN_PIN 27        // Humidity/Temp Fan

// === OLED Setup ===
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// === Global Variables ===
DHT dht(DHTPIN, DHTTYPE);
bool pumpManualState = false;
bool pumpForceOn = false;
bool fanManualState = false;

void setup() {
  Serial.begin(115200);

  // Initialize pins
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Pump OFF
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // Buzzer OFF
  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW); // Fan OFF

  // Init sensors and WiFi
  dht.begin();
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Init OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED init failed"));
    while (true);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Smart Irrigation");
  display.display();
  delay(2000);
}

// === Blynk Controls ===
BLYNK_WRITE(V3) {
  pumpManualState = param.asInt();  // ON/OFF pump control
}

BLYNK_WRITE(V5) {
  pumpForceOn = param.asInt(); // Force pump ON regardless of soil
}

BLYNK_WRITE(V7) {
  fanManualState = param.asInt(); // Manual fan ON/OFF
}

void loop() {
  Blynk.run();

  float temp = dht.readTemperature();
  float humidity = dht.readHumidity();
  int soilRaw = analogRead(SOIL_MOISTURE_PIN);
  float soilPercent = map(soilRaw, 4095, 0, 0, 100);

  int phRaw = analogRead(PH_SENSOR_PIN);
  float voltage = phRaw * (3.3 / 4095.0);
  float pH = 3.5 * voltage;

  int waterLevelRaw = analogRead(WATER_LEVEL_PIN);
  float waterLevelPercent = map(waterLevelRaw, 0, 4095, 0, 100);

  // === Send to Blynk ===
  Blynk.virtualWrite(V0, temp);
  Blynk.virtualWrite(V1, humidity);
  Blynk.virtualWrite(V2, soilPercent);
  Blynk.virtualWrite(V4, pH);
  Blynk.virtualWrite(V6, waterLevelPercent);
  Blynk.virtualWrite(V8, fanManualState); // Optional fan status

  // === OLED Display ===
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Temp: "); display.print(temp); display.println(" C");
  display.print("Hum: "); display.print(humidity); display.println(" %");
  display.print("Soil: "); display.print(soilPercent); display.println(" %");
  display.print("pH: "); display.println(pH, 2);
  display.print("Water: "); display.print(waterLevelPercent); display.println(" %");

  display.print("Pump: ");
  if ((pumpManualState && soilPercent < 40) || pumpForceOn) {
    display.println("ON");
  } else {
    display.println("OFF");
  }

  display.print("Fan: ");
  if (humidity < 60 || temp > 30 || fanManualState) {
    display.println("ON");
  } else {
    display.println("OFF");
  }

  display.display();

  // === Pump Logic ===
  if ((pumpManualState && soilPercent < 40) || pumpForceOn) {
    digitalWrite(RELAY_PIN, LOW);  // Pump ON
  } else {
    digitalWrite(RELAY_PIN, HIGH); // Pump OFF
  }

  // === Buzzer Alert ===
  if (waterLevelPercent < 10) {
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }

  // === Fan Logic ===
  if (humidity < 60 || temp > 30 || fanManualState) {
    digitalWrite(FAN_PIN, HIGH);  // Fan ON
  } else {
    digitalWrite(FAN_PIN, LOW);   // Fan OFF
  }

  delay(2000);
}
