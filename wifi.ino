#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

int statusCode;
int i = 0;

bool connectWifi(void);
void setupAP(void);

ESP8266WebServer server(80);

void setup() {
  Serial.begin(9600);
  delay(100);

  Serial.println("Disconnecting WiFi");
  WiFi.disconnect();

  EEPROM.begin(512);
  delay(10);

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println("Starting...");
  Serial.println("Reading EEPROM for ssid..");

  String EEPROM_SSID;
  for (int i = 0; i < 32; i++) {
    EEPROM_SSID += char(EEPROM.read(i));
  }

  Serial.println("Reading EEPROM for password...");

  String EEPROM_PASSWORD = "";
  for (int i = 32; i < 96; i++) {
    EEPROM_PASSWORD += char(EEPROM.read(i));
  }

  Serial.print("Attempting to connect to SSID: ");
  Serial.print(EEPROM_SSID);
  if (connectWifi()) {
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.print(WiFi.localIP());
    return;
  } else {
    Serial.println("Starting AP...");
    setupAP();
  }
}

void loop() {
  if ((WiFi.status() == WL_CONNECTED)) {
    for (int i = 0; i < 10; i++) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(1000);
      digitalWrite(LED_BUILTIN, LOW);
      delay(1000);
    }
  }
}

bool connectWifi(void) {
  int tries = 0;

  while (tries < 15) {
    if (WiFi.status() == WL_CONNECTED) {
      return true;
    }

    Serial.print(".");
    delay(1000);
    tries++;
  }

  Serial.println("Could not connect to wifi");
  return false;
}

void setupAP(void) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(1000);

  WiFi.softAP("Fuktsensor");
}
