#include <ESP8266React.h>
#include <AsyncTimer.h>
#include <GarageMqttSettingsService.h>
#include <GarageStateService.h>
#include <RFRemoteController.h>

#define SERIAL_BAUD_RATE 115200

// Global Async Timer
AsyncTimer  Timer;

AsyncWebServer server(80);
ESP8266React esp8266React(&server);

GarageMqttSettingsService garageSettingsService = GarageMqttSettingsService(
  &server, esp8266React.getFS(), esp8266React.getSecurityManager()
);

GarageStateService garageService = GarageStateService(
  &server, esp8266React.getSecurityManager(), 
  esp8266React.getMqttClient(), esp8266React.getFS(),
  &garageSettingsService
);

RfRemoteController rfController;

void setup() 
{
  // start serial and filesystem
  Serial.begin(SERIAL_BAUD_RATE);

  // start the framework 
  esp8266React.begin();

  // Start relay stuff
  garageService.begin();
  garageSettingsService.begin();

  // setup rf controller
  rfController.begin();

  // start the server
  server.begin();
}

void loop() {
  // Update global timer
  Timer.handle();

  // run rf loop function
  rfController.loop();

  // run the framework's loop function
  esp8266React.loop();

  // Garage door handling
  garageService.loop();
}
