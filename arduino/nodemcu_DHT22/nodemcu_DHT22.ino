/*
* This program connects NodeMCU Mini to a MQTT broker using TLS
* and sends data from DHT22 sensor to specified topics. It also
* displays sensors data and other info on a 0.96" OLED display.
* 
* Don't forget to set variables to connect to access point and MQTT broker.
* You need to upload client.crt, client.key and ca.crt to Node MCU's file system.
* These need to be issued by your broker's server, as detailed here:
* https://github.com/Aletibazo/TFM/wiki/Aplicaciones-dom%C3%B3tica
* To upload files to the file system, follow this guide: 
* https://github.com/esp8266/arduino-esp8266fs-plugin
* 
* MQTTServer variable must be a domain name, not an IP address.
* IPSET_DNS must be a DNS server that can resolve your broker's name.
* 
* Author: Ale Mosteiro
* Mail:   a.mosteiro@udc.es
*/
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHT.h"

// Temp sensor
#define DHTTYPE DHT22
uint8_t DHTPin = D5;
DHT dht(DHTPin, DHTTYPE);

// OLED
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET  -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Network
const char* ssid = "WIFI_SSID";
const char* password = "SECRETPASSWD";
#define IPSET_STATIC { 192, 168, 0, 30 }
#define IPSET_GATEWAY { 192, 168, 0, 1 }
#define IPSET_SUBNET { 255, 255, 255, 0 }
#define IPSET_DNS { 192, 168, 0, 254 }

byte IPStatic[] = IPSET_STATIC;
byte IPGateway[] = IPSET_GATEWAY;
byte IPSubnet[] = IPSET_SUBNET;
byte IPDNS[] = IPSET_DNS;
const char* ClientName = "ArduBB";
const char* MQTTServer = "your-broker.com";
char* TopicTemp = "topicTemp";
char* TopicHumdt = "topicHumdt";
char* TopicSub = "topicSub";

// Fingerprint of the broker CA
// openssl x509 -in  MQTTServer.crt -sha1 -noout -fingerprint
const char* fingerprint = "HEX fingerprint";
  
WiFiClientSecure ESPClient;
PubSubClient client(MQTTServer, 8883, ESPClient);

long lastMsg = 0;
char msg[50];

float t;
float h;
char buff[5];

int buttonState = 0;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  Serial.begin(115200);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
    Serial.println("SSD1306 allocation failed");
    for(;;);
  }
  delay(2000);

  // Connect WiFi
  oledWrite("Connecting WiFi...");;
  display.display();
  setupWiFi();
  delay(1000);
  // Get time from SNTP Server
  getTime();
  delay(1000);

  // Load certs for TLS
  loadCerts();
  delay(1000);
  
  dht.begin();

  client.setCallback(callback);
}

void loop() {  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();

  // Send sensors data every 5 sec
  if (now - lastMsg > 5000) {
    lastMsg = now;

    t = dht.readTemperature();
    h = dht.readHumidity();
    
    if (isnan(h) || isnan(t)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }
        
    oledTitle();
    display.setCursor(0,20);
    display.setTextSize(2);
    dtostrf(t, 5, 2, buff);
    display.print("T : ");
    display.print(buff);
    display.println("C");
    sendmqttMsg(TopicTemp, buff);
    Serial.println(buff);
    dtostrf(h, 5, 2, buff);
    display.print("HR: ");
    display.print(buff);
    display.println("%");
    sendmqttMsg(TopicHumdt, buff);
    Serial.println(buff);
    display.display();
  }
}

void oledTitle() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 5);
  display.println("Aletibazo Electronics");
}

void oledWrite(char* text) {
  oledTitle();
  display.setCursor(0, 20);
  display.println(text);
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
  oledWrite("WiFi connected");
  Serial.println("IP address: ");
  display.println("IP address: ");
  Serial.println(WiFi.localIP());
  display.println(WiFi.localIP());
  display.display(); 
}

void getTime(){
  // Synchronize time using SNTP. This is necessary to verify that
  // the TLS certificates offered by the server are currently valid.
  oledWrite("Sync time over SNTP");
  display.println("with es.pool.ntp.org");
  display.display();
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

  oledWrite("Time: ");
  display.println(asctime(&timeinfo));
  display.display();
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo)); 
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
    oledWrite("client cert loaded");
    display.display();
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
    display.println("private key loaded");
    display.display();
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
    display.print("ca cert loaded");
    display.display();
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
  oledWrite("Connecting to MQTT broker");
  display.println(MQTTServer);
  display.display();
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
        oledWrite("Connected to ");
        display.println(MQTTServer);
        display.display();
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
