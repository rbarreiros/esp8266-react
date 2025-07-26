#include <AsyncTimer.h>
#include <ESP8266React.h>

#include "GarageSettingsService.h"
#include "GarageStateService.h"
#include "RFRemoteController.h"
#include "RemoteSettingsService.h"
#include "RemoteStateService.h"

#define SERIAL_BAUD_RATE 115200

// Task handles for ESP32 dual-core operation
TaskHandle_t rfTask;
TaskHandle_t espTask;

// Global Async Timer
AsyncTimer Timer;

// Declare pointers instead of static objects
AsyncWebServer* server = nullptr;
ESP8266React* esp8266React = nullptr;
GarageStateService* garageState = nullptr;
GarageSettingsService* garageSettings = nullptr;
RfRemoteController* rfController = nullptr;
RemoteSettingsService* remoteSettings = nullptr;
RemoteStateService* remoteState = nullptr;

void RfControllerTask(void* param)
{
    RfRemoteController *rf = static_cast<RfRemoteController*>(param);

    for(;;)
    {
        rf->loop();
        // Reduced delay due to optimizations
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

void esp8266ReactTask(void *param)
{
    ESP8266React *esp = static_cast<ESP8266React*>(param);
    for(;;)
    {
        esp->loop();
        // Small delay to prevent watchdog issues
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void setup()
{
    // start serial and filesystem
    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println("Starting ESP32 setup...");

    // Simple LED test
    pinMode(2, OUTPUT); // Built-in LED on most ESP32 boards
    digitalWrite(2, HIGH);
    delay(1000);
    digitalWrite(2, LOW);
    Serial.println("LED test complete");

    // Print memory information
    Serial.printf("Free heap before object creation: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Largest free block: %d bytes\n", ESP.getMaxAllocHeap());

    // Create objects dynamically to avoid static initialization issues
    Serial.println("Creating web server...");
    server = new AsyncWebServer(80);
    if (!server) {
        Serial.println("Failed to create web server!");
        return;
    }
    
    Serial.println("Creating ESP8266React framework...");
    esp8266React = new ESP8266React(server);
    if (!esp8266React) {
        Serial.println("Failed to create ESP8266React framework!");
        return;
    }
    
    // Test mode: only create basic objects first
    Serial.println("TEST MODE: Creating only basic objects...");
    
    Serial.println("Creating garage state service...");
    garageState = new GarageStateService(server, esp8266React->getSecurityManager(),
                                        esp8266React->getMqttClient());
    if (!garageState) {
        Serial.println("Failed to create garage state service!");
        return;
    }
    
    Serial.println("Creating garage settings service...");
    garageSettings = new GarageSettingsService(server, esp8266React->getSecurityManager(),
                                              esp8266React->getFS(), garageState);
    if (!garageSettings) {
        Serial.println("Failed to create garage settings service!");
        return;
    }
    
    Serial.printf("Free heap after garage services creation: %d bytes\n", ESP.getFreeHeap());

    Serial.println("Creating RF controller...");
    rfController = new RfRemoteController();
    if (!rfController) {
        Serial.println("Failed to create RF controller!");
        return;
    }
    
    Serial.printf("Free heap after RF controller creation: %d bytes\n", ESP.getFreeHeap());

    Serial.println("Creating remote settings service...");
    remoteSettings = new RemoteSettingsService(server, esp8266React->getSecurityManager(),
                                              esp8266React->getFS(), garageState, rfController);
    if (!remoteSettings) {
        Serial.println("Failed to create remote settings service!");
        return;
    }
    
    Serial.println("Creating remote state service...");
    remoteState = new RemoteStateService(server, esp8266React->getSecurityManager(),
                                        esp8266React->getMqttClient(),
                                        rfController, remoteSettings, garageState);
    if (!remoteState) {
        Serial.println("Failed to create remote state service!");
        return;
    }
    
    Serial.printf("Free heap after remote services creation: %d bytes\n", ESP.getFreeHeap());

    Serial.printf("Free heap after object creation: %d bytes\n", ESP.getFreeHeap());

    // start the framework
    Serial.println("Starting ESP8266React framework...");
    esp8266React->begin();
    Serial.println("ESP8266React framework started");

    /**
     * Start garage door stuff
     * It's important to start garage state
     * first, to configure hardware, and settings
     * after to set settings
     */
    Serial.println("Starting garage services...");
    garageState->begin();
    garageSettings->begin();
    Serial.println("Garage services started");

    // Start remote stuff
    Serial.println("Starting remote services...");
    remoteSettings->begin();
    remoteState->begin();
    Serial.println("Remote services started");

    // setup rf controller
    Serial.println("Setting up RF controller...");
    rfController->begin();
    Serial.println("RF controller setup complete");

    // start the server
    Serial.println("Starting web server...");
    server->begin();
    Serial.println("Web server started");

    // Enable multi-core tasking for optimal performance
    Serial.println("Starting multi-core tasks...");
    
    // start RF Controller task on Core 0 (Arduino core)
    BaseType_t rfTaskCreated = xTaskCreatePinnedToCore(RfControllerTask, "RFController", 8192, rfController, 2, &rfTask, 0);
    if (rfTaskCreated != pdPASS) {
        Serial.println("Failed to create RF Controller task!");
    } else {
        Serial.println("RF Controller task created successfully");
    }

    // ESP web stuff into its own core (Core 1)
    BaseType_t espTaskCreated = xTaskCreatePinnedToCore(esp8266ReactTask, "ESP8266Task", 24576, esp8266React, 1, &espTask, 1);
    if (espTaskCreated != pdPASS) {
        Serial.println("Failed to create ESP8266React task!");
    } else {
        Serial.println("ESP8266React task created successfully");
    }
    
    Serial.println("Multi-core tasking enabled");
    
    Serial.println("Setup complete - entering main loop");
    digitalWrite(2, HIGH); // Turn LED on to indicate setup complete
}

void loop()
{
    static unsigned long lastMemoryPrint = 0;
    static unsigned long loopCount = 0;
    loopCount++;
    
    // Print memory usage every 10 seconds
    if (millis() - lastMemoryPrint >= 10000) {
        Serial.printf("Memory: %d free / %d max alloc | Loop: %lu\n", 
                     ESP.getFreeHeap(), ESP.getMaxAllocHeap(), loopCount);
        lastMemoryPrint = millis();
    }
    
    // RF controller now runs in its own task on Core 0
    // if (rfController) rfController->loop();

    // Update global timer
    // triggers timer functions
    Timer.handle();

    // ESP8266React now runs in its own task on Core 1
    // if (esp8266React) esp8266React->loop();

    // Garage door handling
    // endstop and door status
    if (garageState) garageState->loop();
    
    // Small delay to prevent watchdog issues
    delay(10);
}
