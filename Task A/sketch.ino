#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

#define DHTPIN 4
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

// WiFi
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// MQTT
const char* mqttServer = "broker.emqx.io";
const int mqttPort = 1883;
const char* mqttTopic = "company/interview/env";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Timing
unsigned long lastPublishTime = 0;
unsigned long lastWifiRetry = 0;
unsigned long lastMqttRetry = 0;

const unsigned long PUBLISH_INTERVAL = 5000;
const unsigned long RECONNECT_INTERVAL = 5000;

//--------------------------------------------------
// WiFi Connection
//--------------------------------------------------
void connectWiFi() {

  Serial.println("[WiFi] Connecting...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
}

//--------------------------------------------------
// MQTT Connection
//--------------------------------------------------
bool connectMQTT() {

  String clientId =
      "ESP32Node-" + String(random(1000, 9999));

  Serial.print("[MQTT] Connecting... ");

  if (mqttClient.connect(clientId.c_str())) {

    Serial.println("Connected");

    return true;
  }

  Serial.println("Failed");

  return false;
}

//--------------------------------------------------
// Publish Sensor Data
//--------------------------------------------------
void publishSensorData() {

  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  if (isnan(temp) || isnan(hum)) {

    Serial.println("[Sensor] Read Failed");

    return;
  }

  // Small fluctuation for demo purposes
  temp += random(-20, 20) / 10.0;
  hum += random(-30, 30) / 10.0;

  StaticJsonDocument<256> doc;

  doc["device"] = "ESP32";
  doc["temperature"] = temp;
  doc["humidity"] = hum;
  doc["uptime_ms"] = millis();

  char payload[256];

  serializeJson(doc, payload);

  bool published =
      mqttClient.publish(mqttTopic, payload);

  if (published) {

    Serial.print("[MQTT] Published: ");
    Serial.println(payload);

  } else {

    Serial.println("[MQTT] Publish Failed");
  }
}

//--------------------------------------------------
// Setup
//--------------------------------------------------
void setup() {

  Serial.begin(115200);

  randomSeed(micros());

  dht.begin();

  mqttClient.setServer(
      mqttServer,
      mqttPort
  );

  connectWiFi();
}

//--------------------------------------------------
// Main Loop
//--------------------------------------------------
void loop() {

  unsigned long now = millis();

  // -----------------------------
  // WiFi Reconnect State
  // -----------------------------
  if (WiFi.status() != WL_CONNECTED) {

    if (now - lastWifiRetry >= RECONNECT_INTERVAL) {

      lastWifiRetry = now;

      Serial.println(
          "[WiFi] Reconnection Attempt");

      WiFi.disconnect();

      connectWiFi();
    }

    return;
  }

  // -----------------------------
  // MQTT Reconnect State
  // -----------------------------
  if (!mqttClient.connected()) {

    if (now - lastMqttRetry >= RECONNECT_INTERVAL) {

      lastMqttRetry = now;

      connectMQTT();
    }
  }

  mqttClient.loop();

  // -----------------------------
  // Publish Every 5 Seconds
  // -----------------------------
  if (mqttClient.connected()) {

    if (now - lastPublishTime >= PUBLISH_INTERVAL) {

      lastPublishTime = now;

      publishSensorData();
    }
  }
}