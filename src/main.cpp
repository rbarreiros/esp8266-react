#include <AsyncTimer.h>
#include <ESP8266React.h>

#include "GarageSettingsService.h"
#include "GarageStateService.h"
#include "RFRemoteController.h"
#include "RemoteSettingsService.h"
#include "RemoteStateService.h"

#define SERIAL_BAUD_RATE 115200

TaskHandle_t rfTask;
TaskHandle_t espTask;

// Global Async Timer
AsyncTimer Timer;

AsyncWebServer server(80);
ESP8266React esp8266React(&server);

GarageStateService garageState =
    GarageStateService(&server, esp8266React.getSecurityManager(),
                       esp8266React.getMqttClient());

GarageSettingsService garageSettings =
    GarageSettingsService(&server, esp8266React.getSecurityManager(),
                          esp8266React.getFS(), &garageState);

// Remote control
RfRemoteController rfController;

RemoteSettingsService remoteSettings =
    RemoteSettingsService(&server, esp8266React.getSecurityManager(),
                          esp8266React.getFS(), &garageState, &rfController);

RemoteStateService remoteState =
    RemoteStateService(&server, esp8266React.getSecurityManager(),
                        esp8266React.getMqttClient(),
                       &rfController, &remoteSettings, &garageState);

void RfControllerTask(void* param)
{
    RfRemoteController *rf = static_cast<RfRemoteController*>(param);

    for(;;)
    {
        rf->loop();
    }
}

void esp8266ReactTask(void *param)
{
    ESP8266React *esp = static_cast<ESP8266React*>(param);
    for(;;)
    {
        esp->loop();
    }
}


void setup()
{
    // start serial and filesystem
    Serial.begin(SERIAL_BAUD_RATE);

    // start the framework
    esp8266React.begin();

    /**
     * Start garage door stuff
     * It's important to start garage state
     * first, to configure hardware, and settings
     * after to set settings
     */
    garageState.begin();
    garageSettings.begin();

    // Start remote stuff
    remoteSettings.begin();
    remoteState.begin();

    // setup rf controller
    rfController.begin();

    // start the server
    server.begin();


    // start RF Controller task
    //xTaskCreatePinnedToCore(RfControllerTask, "RFController", 5000, &rfController, 0, &rfTask, 0);

    // ESP web stuff into its own core
    //xTaskCreatePinnedToCore(esp8266ReactTask, "ESP8266Task", 50000, &esp8266React, 0, &espTask, 0);
}

void loop()
{
    // run rf loop function
    // receives remote codes
    //  this should really run very fast!!!
    rfController.loop();

    // Update global timer
    // triggers timer functions
    Timer.handle();

    // run the framework's loop function
    esp8266React.loop();

    // Garage door handling
    // endstop and door status
    garageState.loop();
}
