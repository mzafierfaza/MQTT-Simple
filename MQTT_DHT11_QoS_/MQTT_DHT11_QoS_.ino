#include "DHT.h"
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>

#define WIFI_SSID "ulfa"
#define WIFI_PASSWORD "akubaiknian"

#define MQTT_HOST IPAddress(192, 168, 43, 243) // raspi Sari

#define MQTT_PORT 1883
#define MQTT_PUB_TEMP "esp/dht/temperature"

#define DHTPIN 14 // D5
#define DHTTYPE DHT11   // DHT 11


DHT dht(DHTPIN, DHTTYPE);

float temp;
float hum;

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;


void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Connected to Wi-Fi.");
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttPublish(uint16_t packetId) {
  Serial.print("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  dht.begin();
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  connectToWifi();
}

void loop() {
  temp = dht.readTemperature();
  // Publish an MQTT message on topic esp/dht11/temperature
  uint16_t packetIdPub1 = mqttClient.publish(MQTT_PUB_TEMP, 2, true, String(temp).c_str());
  Serial.printf("Publishing on topic %s at QoS 0, packetId: %i ", MQTT_PUB_TEMP, packetIdPub1);
  Serial.printf("Message: %.2f \n", temp);

  // ---- Parameter -----
  /*MQTT topic (const char*)
    QoS (uint8_t): quality of service – it can be 0, 1 or 2
    retain flag (bool): retain flag
    payload (const char*) – in this case, the payload corresponds to the sensor reading
  */
  // ---- Pengertian ----
  /*The QoS (quality of service) is a way to guarantee that the message is delivered. It can be one of the following levels:
    0: the message will be delivered once or not at all. The message is not acknowledged. There is no possibility of duplicated messages;
    1: the message will be delivered at least once, but may be delivered more than once;
    2: the message is always delivered exactly once;
  */
}
