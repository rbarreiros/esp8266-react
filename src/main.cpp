#include <AsyncTimer.h>
#include <ESP8266React.h>
#include <MemoryManager.h>

#include "GarageSettingsService.h"
#include "GarageStateService.h"
#include "RFRemoteController.h"
#include "RemoteSettingsService.h"
#include "RemoteStateService.h"

#define SERIAL_BAUD_RATE 115200

//TaskHandle_t rfTask;
//TaskHandle_t espTask;

// Global Async Timer
AsyncTimer Timer;

// Global pointers - will be allocated in setup()
AsyncWebServer* server = nullptr;
ESP8266React* esp8266React = nullptr;
GarageStateService* garageState = nullptr;
GarageSettingsService* garageSettings = nullptr;
RfRemoteController* rfController = nullptr;
RemoteSettingsService* remoteSettings = nullptr;
RemoteStateService* remoteState = nullptr;

/*
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

// Usage (if needed):
// xTaskCreatePinnedToCore(RfControllerTask, "RFController", 5000, rfController, 0, &rfTask, 0);
// xTaskCreatePinnedToCore(esp8266ReactTask, "ESP8266Task", 50000, esp8266React, 0, &espTask, 0);
*/

void setup()
{
    // start serial and filesystem
    Serial.begin(SERIAL_BAUD_RATE);

    Serial.println("Starting ESP React System");
    
    // Allocate global objects in proper order
    Serial.println("Allocating server and framework objects");
    server = new AsyncWebServer(80);
    esp8266React = new ESP8266React(server);
    
    Serial.println("Starting ESP React framework");
    esp8266React->begin();

    /**
     * Start garage door stuff
     * It's important to start garage state
     * first, to configure hardware, and settings
     * after to set settings
     */
    Serial.println("Allocating garage objects");
    garageState = new GarageStateService(server, esp8266React->getSecurityManager(),
                                        esp8266React->getMqttClient());
    
    garageSettings = new GarageSettingsService(server, esp8266React->getSecurityManager(),
                                              esp8266React->getFS(), garageState);

    Serial.println("Starting garage services");
    garageState->begin();
    garageSettings->begin();

    // Start remote stuff
    Serial.println("Allocating remote objects");
    rfController = new RfRemoteController();
    
    remoteSettings = new RemoteSettingsService(server, esp8266React->getSecurityManager(),
                                              esp8266React->getFS(), garageState, rfController);
    
    remoteState = new RemoteStateService(server, esp8266React->getSecurityManager(),
                                        esp8266React->getMqttClient(),
                                        rfController, remoteSettings, garageState);

    Serial.println("Starting remote services");
    remoteSettings->begin();
    remoteState->begin();

    // setup optimized rf controller
    Serial.println("Starting optimized rfController");
    rfController->begin();

    // start the server
    Serial.println("Starting server");
    server->begin();

    Serial.println("Starting main loop");
    
    // Display initial memory status
    MemoryManager::getInstance().dumpMemoryInfo();
}

// Shutdown handler for clean memory leak detection
void __attribute__((destructor)) cleanup() {
    Serial.println("Application shutting down...");
    
    // Clean up dynamically allocated objects
    if (remoteState) {
        delete remoteState;
        remoteState = nullptr;
    }
    if (remoteSettings) {
        delete remoteSettings;
        remoteSettings = nullptr;
    }
    if (rfController) {
        delete rfController;
        rfController = nullptr;
    }
    if (garageSettings) {
        delete garageSettings;
        garageSettings = nullptr;
    }
    if (garageState) {
        delete garageState;
        garageState = nullptr;
    }
    if (esp8266React) {
        delete esp8266React;
        esp8266React = nullptr;
    }
    if (server) {
        delete server;
        server = nullptr;
    }
    
    MemoryManager::getInstance().reportLeaks();
    MemoryManager::getInstance().end();
}

void loop()
{
    static unsigned long lastRfTime = 0;
    static unsigned long lastFrameworkTime = 0;
    static unsigned long lastGarageTime = 0;
    static unsigned long lastStatsTime = 0;
    
    unsigned long now = millis();
    
    // RF controller runs every loop (highest priority, optimized)
    if (rfController) {
        rfController->loop();
    }
    
    // Timer handling
    Timer.handle();
    
    // Framework operations every 10ms
    if (esp8266React && now - lastFrameworkTime >= 10) {
        esp8266React->loop();
        lastFrameworkTime = now;
    }
    
    // Garage state every 50ms
    if (garageState && now - lastGarageTime >= 50) {
        garageState->loop();
        lastGarageTime = now;
    }
    
    // Performance monitoring every 30 seconds
    if (now - lastStatsTime >= 30000) {
        // RF Controller statistics
        if (rfController) {
            auto rfStats = rfController->getStatistics();
            Serial.printf("RF Stats: RX=%lu, Processed=%lu, Dropped=%lu, Duplicates=%lu, Timeouts=%lu, Overflows=%lu, AvgTime=%luus, MaxTime=%luus\n",
                          rfStats.packetsReceived, rfStats.packetsProcessed, rfStats.packetsDropped, 
                          rfStats.duplicatesFiltered, rfStats.timeouts, rfStats.bufferOverflows,
                          rfStats.averageProcessingTime, rfStats.maxProcessingTime);
        }
        
        // Memory statistics
        auto memStats = MemoryManager::getInstance().getCurrentStats();
        Serial.printf("Memory Stats: Free=%u, Total=%u, MaxAlloc=%u, MinFree=%u, Frag=%u%%, Stack=%u, Malloc=%lu, Free=%lu, Failed=%lu\n",
                      memStats.freeHeap, memStats.totalHeap, memStats.maxAllocHeap, memStats.minFreeHeap,
                      memStats.heapFragmentation, memStats.freeStack, memStats.mallocCount, 
                      memStats.freeCount, memStats.failedAllocCount);
        
        // Memory pressure indicators
        if (memStats.lowMemoryEvents || memStats.criticalMemoryEvents || memStats.fragmentationEvents) {
            Serial.printf("Memory Events: Low=%lu, Critical=%lu, Fragmentation=%lu\n",
                          memStats.lowMemoryEvents, memStats.criticalMemoryEvents, memStats.fragmentationEvents);
        }
        
        lastStatsTime = now;
    }
}
