#include <ArduinoJson.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "arduino_secrets.h"
#include <WiFiManager.h> 

#define DHTTYPE DHT11
#define DHTPIN 2

// DHT11 dht11(2);  // ESP8266 D4
DHT dht(DHTPIN, DHTTYPE, 11);
WiFiManager wifiManager;
WiFiClient espClient;
PubSubClient client(espClient);

char ssid[] = SECRET_SSID;
char password[] = SECRET_PASS;
char mqttUser[] = SECRET_MQTTUSER;
char mqttPassword[] = SECRET_MQTTPASS;
const char hostname[] = "Wemos_D1_Mini_DH11";
const char broker[] = SECRET_MQTTBROKER;
String stateTopic = "home/storage/" + String(hostname) + "/state";
int port = SECRET_MQTTPORT;

void setup() {
  Serial.begin(9600);
  delay(100);

  dht.begin();

  wifiManager.setClass("invert");

  // is configuration portal requested?
  if ( digitalRead(TRIGGER_PIN) == LOW ) {
    WiFiManager wifiManager;
    wifiManager.startConfigPortal("OnDemandAP");
    Serial.println("connected...yeey :)");
  }

  Serial.print("Attempting to connect to SSID: ");
  Serial.print(ssid);
  Serial.print(" as ");
  Serial.print(hostname);

  WiFi.hostname(hostname);
  WiFi.mode(WIFI_STA);
  WiFi.begin(SECRET_SSID, SECRET_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  WiFi.setAutoReconnect(true);

  Serial.println("");
  Serial.println("WiFi connected");

  Serial.print("IP address: ");
  Serial.print(WiFi.localIP());

  Serial.println("");
  Serial.print("Attempting to connect to MQTT broker: ");
  Serial.println(broker);

  client.setServer(broker, port);
  client.setBufferSize(512);

  while (!client.connected()) {
    Serial.print(".");

    if (client.connect(hostname, mqttUser, mqttPassword)) {
      Serial.println("");
      Serial.println("MQTT connected");

      sendMQTTTemperatureDiscoveryMsg();
      sendMQTTHumidityDiscoveryMsg();
    } else {
      Serial.print("MQTT connection failed! Error code = ");
      Serial.println(client.state());
    }
  }
}

void sendMQTTTemperatureDiscoveryMsg() {
  String discoveryTopic = "homeassistant/sensor/" + String(hostname) + "_temperature/config";

  DynamicJsonDocument doc(1024);
  char buffer[256];

  doc["name"] = String(hostname) + "_temperature";
  doc["stat_t"] = stateTopic;
  doc["unit_of_meas"] = "°C";
  doc["dev_cla"] = "temperature";
  doc["frc_upd"] = true;
  doc["uniq_id"] = String(hostname) + "_temperature";
  doc["val_tpl"] = "{{value_json.temperature | round(3) | default(0)}}";

  size_t n = serializeJson(doc, buffer);

  client.publish(discoveryTopic.c_str(), buffer, n);
}

void sendMQTTHumidityDiscoveryMsg() {
  String discoveryTopic = "homeassistant/sensor/" + String(hostname) + "_humidity/config";

  DynamicJsonDocument doc(1024);
  char buffer[256];

  doc["name"] = String(hostname) + "_humidity";
  doc["stat_t"] = stateTopic;
  doc["unit_of_meas"] = "%";
  doc["dev_cla"] = "humidity";
  doc["frc_upd"] = true;
  doc["uniq_id"] = String(hostname) + "_humidity";
  doc["val_tpl"] = "{{value_json.humidity | round(3) | default(0)}}";

  size_t n = serializeJson(doc, buffer);

  client.publish(discoveryTopic.c_str(), buffer, n);
}

float temperature = 0.0;
float humidity = 0.0;
float diff = 1.0;

bool checkBound(float newValue, float prevValue, float maxDiff) {
  return !isnan(newValue) && (newValue < prevValue - maxDiff || newValue > prevValue + maxDiff);
}

void loop() {
  float newTemp = dht.readTemperature();
  float newHum = dht.readHumidity();

  if (checkBound(newTemp, temperature, diff)) {
    temperature = newTemp;
  }

  if (checkBound(newHum, humidity, diff)) {
    humidity = newHum;
  }

  DynamicJsonDocument doc(1024);
  char buffer[256];

  doc["temperature"] = temperature;
  doc["humidity"] = humidity;

  size_t n = serializeJson(doc, buffer);

  bool published = client.publish(stateTopic.c_str(), buffer, n);

  Serial.println("Published: ");
  Serial.println(published);

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");

  delay(5000);
}
