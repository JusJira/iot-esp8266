#include "secrets_data.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "DHT.h"

#define DHTPIN D1
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

// MQTT-Connection
WiFiClient client;
PubSubClient mqtt(client);

// MQTT-Callback Function
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  WiFi.reconnect();
  // Loop until we're reconnected
  Serial.println(mqtt.connected());
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (mqtt.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("connected");
      mqtt.subscribe("esp8266/sensors/");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setupNetwork() {
  WiFi.begin(ssid, pass);                  //ทำการ Connect SSID และ Pass
  while (WiFi.status() != WL_CONNECTED) {  // ถ้าไม่สามารถเชื่อมต่อได้
    // ทำการ Print "Connectiong..." ทุก 1000ms
    Serial.println("Connecting...  ");
    // แสดงสถานะการเชื่อมต่อ
    Serial.printf("Connection Status: %d\n", WiFi.status());
    delay(1000);
  }
  Serial.print("Wi-Fi connected.");
  Serial.print("IP Address : ");
  Serial.println(WiFi.localIP());  // ทำการ Print IP ที่ได้รับมาจาก
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
}

void setup() {
  Serial.begin(115200);
  setupNetwork();
  dht.begin();
  //MQTT-Setup
  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setCallback(callback);
}

void loop() {
  static uint32_t prev_millis = 0;
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  char json_body[100];
  const char json_tmpl[] = "{\"temperature\": %.2f,"
                           "\"humidity\": %.2f}";

  if (!mqtt.connected()) {
    reconnect();
  }
  if (millis() - prev_millis > 15000) {
    prev_millis = millis();
    sprintf(json_body, json_tmpl, t, h);
    Serial.println(json_body);
    mqtt.publish("esp8266/sensors/", json_body);
  }
  mqtt.loop();
  delay(1000);
}