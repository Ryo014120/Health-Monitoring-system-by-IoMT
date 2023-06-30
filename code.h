#include <WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <AD8232.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// WiFi Configuration
const char* WIFI_SSID = "your_wifi_ssid";
const char* WIFI_PASSWORD = "your_wifi_password";

// MQTT Configuration
const char* MQTT_BROKER = "broker.example.com";
const int MQTT_PORT = 1883;
const char* MQTT_TOPIC = "sensor/data";
const char* MQTT_CLIENT_ID = "ESP32Client";

// Ubidots Configuration
const char* UBIDOTS_TOKEN = "your_ubidots_token";
const char* UBIDOTS_VARIABLE_ID = "your_ubidots_variable_id";

// Define the temperature sensor
#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature temperatureSensor(&oneWire);

// Define the ECG sensor
const int ecgPin = 33; // AD8232 output connected to ESP32 GPIO 33

// Define the heart pulse sensor
Adafruit_ADS1115 ads; // Create an instance of ADS1115
const int adsPin = 0; // ADS1115 analog input connected to A0 pin of Easy Pulse V1.1


// LCD Configuration
const int LCD_ADDRESS = 0x27;
const int LCD_COLUMNS = 16;
const int LCD_ROWS = 2;

// Initialize MQTT client
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// MQTT Publish-Subscribe Functions
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Handle MQTT message
  Serial.print("Received MQTT message: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void mqttPublish(const char* topic, const char* data) {
  mqttClient.publish(topic, data);
}

void setupMQTT() {
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
}

void connectMQTT() {
  while (!mqttClient.connected()) {
    Serial.println("Connecting to MQTT broker...");
    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      Serial.println("Connected to MQTT broker");
      mqttClient.subscribe(MQTT_TOPIC);
    } else {
      Serial.print("Failed to connect to MQTT broker. Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

// DS18B20 Temperature Sensor Functions
OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature tempSensor(&oneWire);

float readTemperature() {
  tempSensor.requestTemperatures();
  float temperature = tempSensor.getTempCByIndex(0);
  return temperature;
}

// ECG Functions
void setupECG() {
  ad8232.begin();
}

int readECG() {
  int ecgValue = ad8232.readECG();
  return ecgValue;
}

// Heart Pulse Functions
int readHeartPulse() {
  int pulseValue = analogRead(PULSE_SENSOR_PIN);
  return pulseValue;
}

// LCD Functions
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);

void setupLCD() {
  lcd.begin(LCD_COLUMNS, LCD_ROWS);
}

void lcdPrint(const char* message, int column, int row) {
  lcd.setCursor(column, row);
  lcd.print(message);
}

void clearLCD() {
  lcd.clear();
}

// Ubidots Functions
void sendToUbidots(const char* variableId, const char* value) {
  char payload[100];
  sprintf(payload, "{\"%s\": %s}", variableId, value);
  mqttPublish("/v1.6/devices/esp32", payload);
}

void setup() {
  Serial.begin(9600);
  delay(100);

  // Connect to Wi-Fi
  Serial.println();
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to Wi-Fi!");

  // Setup MQTT
  setupMQTT();

  // Setup DS18B20 Temperature Sensor
  tempSensor.begin();

  // Setup ECG
  setupECG();

  // Setup LCD
  setupLCD();
}

void loop() {
  if (!mqttClient.connected()) {
    connectMQTT();
  }
  mqttClient.loop();

  // Read and publish ECG data
  int ecgValue = readECG();
  char ecgData[10];
  sprintf(ecgData, "%d", ecgValue);
  mqttPublish(MQTT_TOPIC, ecgData);
  sendToUbidots(UBIDOTS_VARIABLE_ID, ecgData);

  // Read and publish Heart Pulse data
  int pulseValue = readHeartPulse();
  char pulseData[10];
  sprintf(pulseData, "%d", pulseValue);
  mqttPublish(MQTT_TOPIC, pulseData);
  sendToUbidots(UBIDOTS_VARIABLE_ID, pulseData);

  // Read and publish Body Temperature data
  float temperature = readTemperature();
  char tempData[10];
  sprintf(tempData, "%.2f", temperature);
  mqttPublish(MQTT_TOPIC, tempData);
  sendToUbidots(UBIDOTS_VARIABLE_ID, tempData);

  // Display data on LCD
  clearLCD();
  lcdPrint(ecgData, 0, 0);
  lcdPrint(pulseData, 0, 1);
  delay(2000);
}
