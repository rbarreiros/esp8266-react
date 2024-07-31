#include <ESP8266React.h>
#include <RCSwitch.h>
#include <AsyncTimer.h>

#include <RelayMqttSettingsService.h>
#include <RelayStateService.h>
#include <LightMqttSettingsService.h>
#include <LightStateService.h>

#define SERIAL_BAUD_RATE 115200

#define LED_PIN 16
#define RELAY_PIN 5
#define RF_PIN 4

// Global Adync Timer
AsyncTimer  Timer;


AsyncWebServer server(80);
ESP8266React esp8266React(&server);

RelayMqttSettingsService relaySettingsService = RelayMqttSettingsService(
  &server, esp8266React.getFS(), esp8266React.getSecurityManager()
);

RelayStateService relayService = RelayStateService(
  &server, esp8266React.getSecurityManager(), esp8266React.getMqttClient(), &relaySettingsService
);

LightMqttSettingsService lightSettingsService = LightMqttSettingsService(
  &server, esp8266React.getFS(), esp8266React.getSecurityManager()
);

LightStateService lightService = LightStateService(
  &server, esp8266React.getSecurityManager(), esp8266React.getMqttClient(), &lightSettingsService
);

RCSwitch mySwitch = RCSwitch();

void setup() {
  // start serial and filesystem
  Serial.begin(SERIAL_BAUD_RATE);

  // start the framework 
  esp8266React.begin();

  // Start relay stuff
  relayService.begin();
  relaySettingsService.begin();
  lightService.begin();
  lightSettingsService.begin();

  // start the server
  server.begin();

  // TODO
  // Start rf receiver
  mySwitch.enableReceive(4); // GPIO4 / D2 i guess
}

void loop() {
  // Update global timer
  Timer.handle();

  // run the framework's loop function
  esp8266React.loop();

  if(mySwitch.available())
  {
    Serial.printf("Received value %ld\r\n", mySwitch.getReceivedValue());
    mySwitch.resetAvailable();
  }
}
