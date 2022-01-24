/*
* This is the firmware for Aletibazo Electronics Sensor Board v0.1
* 
* Pinout correspondence between the board and the ESP8266:
*   * PIR      -> D0
*   * LED      -> D3
*   * 1W, TEMP -> D4
*   * HUMIDITY -> D5
*   
* This program connects to a MQTT broker using TLS
* and sends data from DS18B20, DHT22 and PIR sensors to specified topics. It also
* displays sensors data and other info on a 128x32 OLED display.
* 
* Don't forget to set variables to connect to access point and MQTT broker.
* You need to upload client.crt, client.key and ca.crt to Node MCU's file system.
* These need to be issued by your broker's server, as detailed here:
* https://gitlab.citic.udc.es/a.mosteiro/tfm/-/wikis/Aplicaciones-dom%C3%B3tica
* To upload files to the file system, follow this guide: 
* https://github.com/esp8266/arduino-esp8266fs-plugin
* 
* Note that MQTTServer variable must be a domain name, not an IP address.
* IPSET_DNS must be a DNS server that can resolve your broker's name.
* 
* Author: Ale Mosteiro
* Mail:   a.mosteiro@udc.es
* 
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <DHTesp.h>

// PIR sensor is connected to pin D0
int PIR_SENSOR = D0;
// DHT11 sensor connected to pin D5
int DHT_SENSOR = D5;
DHTesp dht;

// Data wire is plugged into pin D4 on the NodeMCU
#define ONE_WIRE_BUS D4
// Setup a oneWire instance to communicate with any OneWire devices 
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// Set OLED type
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0);

/**
 * Customize this settings 
 */
const char* ssid = "SSID-Name";
const char* password = "SSID-Password";
#define IPSET_STATIC { xxx, xxx, xxx, xxx }
#define IPSET_GATEWAY { xxx, xxx, xxx, xxx }
#define IPSET_SUBNET { xxx, xxx, xxx, xxx }
#define IPSET_DNS { xxx, xxx, xxx, xxx }


// TODO Change this to parametrize ClientName so it needs to be replaced only once
const char* ClientName = "RackNumber";
const char* MQTTServer = "your-domain.org";

char* TopicSub = "cpd/RackNumber/led";
char* TopicConfigTemp = "cpd/sensor/RackNumberTemp/config";
char* PayloadConfigTemp = "{\"device_class\": \"temperature\", \"name\": \"RackNumberTemperature\", \"state_topic\": \"cpd/sensor/RackNumber/state\", \"unit_of_measurement\": \"°C\", \"value_template\": \"{{ value_json.temperature }}\" }";
char* TopicConfigHumdt = "cpd/sensor/RackNumberHumdt/config";
char* PayloadConfigHumdt = "{\"device_class\": \"humidity\", \"name\": \"RackNumberHumidity\", \"state_topic\": \"cpd/sensor/RackNumber/state\", \"unit_of_measurement\": \"%\", \"value_template\": \"{{ value_json.humidity }}\" }";
char* StateTopic = "cpd/sensor/RackNumber/state";
char* TopicConfigPIR = "cpd/binary_sensor/RackNumberPIR/config";
char* PayloadConfigPIR = "{\"device_class\": \"motion\", \"name\": \"RackNumberPIR\", \"state_topic\": \"cpd/binary_sensor/RackNumberPIR/state\" }";
char* StateTopicPIR = "cpd/binary_sensor/RackNumberPIR/state";

/**
 * End of customization
 */

byte IPStatic[] = IPSET_STATIC;
byte IPGateway[] = IPSET_GATEWAY;
byte IPSubnet[] = IPSET_SUBNET;
byte IPDNS[] = IPSET_DNS;
  
WiFiClientSecure ESPClient;
PubSubClient client(MQTTServer, 8883, ESPClient);

int lastMsg = 0;
boolean configured = false;

char msg[50];

float t;
float h;
char buff[5];

int buttonState = 0;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  
  Serial.begin(115200);

  u8g2.begin();
  dht.setup(DHT_SENSOR, DHTesp::DHT11);

  printInfo("Setting up...");
  // Connect WiFi
  printInfo("Connecting WiFi");
  setupWiFi();
  delay(1000);
  // Get time from SNTP Server
  printInfo("Get SNTP time");
  getTime();
  delay(1000);

  // Load certs for TLS
  printInfo("Load certs");
  loadCerts();
  delay(1000);
  
  //dht.begin();
  sensors.begin();

  client.setCallback(callback);
}

void loop() {  
  if (!client.connected()) {
    printInfo("Connect broker");
    reconnect();
  }
  client.loop();

  long now = millis();

  // Send sensor config on power on
  if (!configured) {
    configured = true;

    sendmqttMsg(TopicConfigTemp, PayloadConfigTemp);
    sendmqttMsg(TopicConfigHumdt, PayloadConfigHumdt);
    sendmqttMsg(TopicConfigPIR, PayloadConfigPIR);
  }
  
  // Send sensors data every 5 sec
  if (now - lastMsg > 5000) {
    String payload = String();
    lastMsg = now;

    // Obtain temperature from DS18B20 1-wire sensor
    sensors.requestTemperatures();
    t = sensors.getTempCByIndex(0);
    // Add temperature to payload
    payload = "{ \"temperature\": " + String(t, 2);
    // Obtain humidity from DHT22 sensor
    h = dht.getHumidity();
    // If we can't get data from the sensor, we send -127
    String hmdt = (isnan(h) == 1) ? "-127" : String(h, 2);
    // Add humidity to payload
    payload = payload + ", \"humidity\": ";
    payload = payload + hmdt;
    // Add presence to payload, getting value from digital pin
    // Close payload string, send it to StateTopic MQTT topic
    payload = payload + " }";
    // Send MQTT payload for temperature and humidity
    sendmqttMsg(StateTopic, payload);

    // Prepare payload for PIR, reading data from DHT22
    String motion = (digitalRead(PIR_SENSOR) == 1) ? "ON" : "OFF";
    payload = "{ \"state\": \"" + motion;
    // Close payload string, send it to StateTopic MQTT topic
    payload = payload + "\" }";
    // Send MQTT payload for PIR
    sendmqttMsg(StateTopicPIR, motion);
    
    
    // Print data to OLED
    printData(t, h);
    
  }
}

// Function to display set up information
// Given char must be 16chars max
void printInfo(const char *s){
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_t0_16_tf);
  u8g2.drawStr(0, 12, ClientName);
  u8g2.drawStr(0, 24, s);
  u8g2.sendBuffer();
}
// Function to display set up information
// Given char must be 16chars max
void printData(float temperature, float humidity){
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_t0_16_tf);
  u8g2.setCursor(0, 12);
  u8g2.print(ClientName);
  u8g2.setFont(u8g2_font_profont10_tf);
  u8g2.setCursor(0, 20);
  u8g2.print("T: ");
  u8g2.print(temperature);
  u8g2.print(" C");
  u8g2.setCursor(0, 28);
  u8g2.print("H: ");
  u8g2.print(humidity);
  u8g2.print(" %");
  u8g2.sendBuffer();
}


void setupWiFi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.disconnect();

  WiFi.begin(ssid, password);
  WiFi.config(IPAddress(IPStatic), IPAddress(IPGateway), IPAddress(IPSubnet), IPAddress(IPDNS));
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void getTime(){
  // Synchronize time using SNTP. This is necessary to verify that
  // the TLS certificates offered by the server are currently valid.
  Serial.print("Setting time using SNTP");
  configTime(8 * 3600, 0, "es.pool.ntp.org");
  time_t now = time(nullptr);
  while (now < 1000) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);

  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
  printInfo(asctime(&timeinfo));
}

// Load Certificates
void loadCerts() {
  
  if (!SPIFFS.begin()) {
   Serial.println("Failed to mount file system");
   return;
  }
  
  // Load client certificate file from SPIFFS
  File cert = SPIFFS.open("/client.crt", "r"); //replace client.crt with your uploaded file name
  if (!cert)
    Serial.println("Failed to open cert file");
  else
    Serial.println("Success to open cert file");

  delay(1000);

  // Set client certificate
  if (ESPClient.loadCertificate(cert)){
    Serial.println("cert loaded");
  } else
    Serial.println("cert not loaded");

  // Load client private key file from SPIFFS
  File private_key = SPIFFS.open("/client.key", "r"); //replace client.key with your uploaded file name
  if (!private_key)
    Serial.println("Failed to open private cert file");
  else
    Serial.println("Success to open private cert file");

  delay(1000);

  // Set client private key
  if (ESPClient.loadPrivateKey(private_key)){
    Serial.println("private key loaded");
  }
  else
    Serial.println("private key not loaded");

  // Load CA file from SPIFFS
  File ca = SPIFFS.open("/ca.crt", "r"); //replace ca.crt with your uploaded file name
  if (!ca)
    Serial.println("Failed to open ca ");
  else
    Serial.println("Success to open ca");
  delay(1000);

  // Set server CA file
  if(ESPClient.loadCACert(ca)){
    Serial.println("ca loaded");
  } else
    Serial.println("ca failed");
}

// Callback function to activate LED if there are errors
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

// Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1')
    digitalWrite(LED_BUILTIN, LOW);
  else
    digitalWrite(LED_BUILTIN, HIGH);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(ClientName)) {
        delay(1000);
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("connected", ClientName);
      // ... and / or resubscribe
      client.subscribe(TopicSub);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void sendmqttMsg(char* topictosend, String payload) {

  if (client.connected()) {
      Serial.print("Sending payload: ");
      Serial.print(payload);

    unsigned int msg_length = payload.length();

      Serial.print(" length: ");
      Serial.println(msg_length);

    byte* p = (byte*)malloc(msg_length);
    memcpy(p, (char*) payload.c_str(), msg_length);

    if ( client.publish(topictosend, p, msg_length)) {
        Serial.println("Publish ok");
      free(p);
      //return 1;
    } else {
        Serial.println("Publish failed");
      free(p);
      //return 0;
    }
  }
}
