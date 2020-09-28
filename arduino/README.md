# TFM_arduinos
NodeMCU code to connect to a MQTT broker securely using TLS and sending sensors data to specified topics and displaying in on a 0.96" OLED.

You need to upload client certificate and private key as well as CA certificate into your NodeMCU's file system. Do this using ESP8266 filesystem uploader:
https://github.com/esp8266/arduino-esp8266fs-plugin
NOTE: these files must have read permissions.

Inside nodemcu_DHT22 there's code to configure a DHT22 temperature & humidity sensor.

Detailed instrucctions can be found inside each program's code.