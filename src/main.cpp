#include <ESP8266React.h>
#include <AsyncTimer.h>
#include <GarageMqttSettingsService.h>
#include <GarageStateService.h>

#include <RemoteSettingsService.h>
#include <RFRemoteController.h>

#define SERIAL_BAUD_RATE 115200

// Global Async Timer
AsyncTimer  Timer;

AsyncWebServer server(80);
ESP8266React esp8266React(&server);

// Gate control
GarageMqttSettingsService garageSettingsService = GarageMqttSettingsService(
  &server, esp8266React.getFS(), esp8266React.getSecurityManager()
);

GarageStateService garageService = GarageStateService(
  &server, esp8266React.getSecurityManager(), 
  esp8266React.getMqttClient(), esp8266React.getFS(),
  &garageSettingsService
);

// Remote control
RfRemoteController rfController;

RemoteSettingsService remoteService = RemoteSettingsService(
  &server, esp8266React.getSecurityManager(), 
  esp8266React.getFS(), &garageService, &rfController
);


void setup() 
{
  // start serial and filesystem
  Serial.begin(SERIAL_BAUD_RATE);

  // start the framework 
  esp8266React.begin();

  // Start relay stuff
  garageService.begin();
  garageSettingsService.begin();

  // Start remote stuff
  remoteService.begin();

  // setup rf controller
  rfController.begin();

  // start the server
  server.begin();
}

void loop() {
  // Update global timer
  // triggers timer functions
  Timer.handle();

  // run rf loop function
  // receives remote codes
  rfController.loop();

  // run the framework's loop function
  esp8266React.loop();

  // Garage door handling
  // endstop status
  garageService.loop();
}
