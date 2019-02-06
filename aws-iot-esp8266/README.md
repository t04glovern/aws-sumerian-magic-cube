# AWS IoT - Espressif X.509 MQTT Publisher of MMA8451 Data

This project is setup for performing basic FFT on movements in order to recognise events of high movement magnitude

## Setup

Create the `src/main.h` file based on `src/main.h.example` with the relevant information for your project and save it.

```cpp
#ifndef MAIN_H

// Wifi Details
const char *ssid = "YourWifiSSID";
const char *password = "YourWifiPassword";

const String thing_id = "YourThingID";

// AWS MQTT Details
char *aws_mqtt_server = "YourAWSThingID.iot.us-east-1.amazonaws.com";
char *aws_mqtt_client_id = "YourMQTTClientID";
char *aws_mqtt_thing_topic_pub = "Your/MQTT/Topic";
char *aws_mqtt_thing_topic_sub = "Your/MQTT/Topic";

#endif
```

## Uploading Certificates

You will also need to create the cert files based on the output from the CloudFormation deploy of the vending machine

```bash
openssl x509 -in aws/certs/certificate.pem.crt -out data/cert.der -outform DER
openssl rsa -in aws/certs/private.pem.key -out data/private.der -outform DER
openssl x509 -in aws/certs/root-CA.pem -out data/ca.der -outform DER
```

Then upload the certificates using SPIFFS

```bash
pio run -t uploadfs
```

## MQTT Providers

### AWS IoT

You can sign up for and follow along with the setup guides on [AWS IoT Setup](https://us-east-1.console.aws.amazon.com/iotv2/home?region=us-east-1#/connIntro)

## Platform IO

This project is build and run with PlatformIO. The library dependencies can be found in the `platformio.ini` file. Below is the current configuration targetting the FireBeetle ESP32 development board. This can be changed to any variable of the ESP32 chip.

```ini
[env:nodemcuv2]
platform = espressif8266
board = nodemcuv2
framework = arduino
monitor_speed = 115200

lib_deps =
  ArduinoJson@5.13.1
  PubSubClient@2.7
  Adafruit MMA8451 Library@1.0.5
  arduinoFFT@1.4
```

### References

- [Some examples using x.509 certificates and TLSv1.2 under Arduino IDE](https://github.com/copercini/esp8266-aws_iot)
- [IoT Thing, Shadow, and Script Actions](https://docs.sumerian.amazonaws.com/tutorials/create/intermediate/iot-thing-shadow-script/)
