#ifndef WiFiScanner_h
#define WiFiScanner_h

#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif

#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>
#include <SecurityManager.h>
#include <MemoryManager.h>
#include <memory>

#define SCAN_NETWORKS_SERVICE_PATH "/rest/scanNetworks"
#define LIST_NETWORKS_SERVICE_PATH "/rest/listNetworks"

class WiFiScanner 
{
public:
  WiFiScanner(AsyncWebServer* server, SecurityManager* securityManager);
  void loop();  // Called from main loop to check scan completion

private:
  AsyncWebServer* _server;
  SecurityManager* _securityManager;
  std::unique_ptr<AsyncWebSocket> _scanWebSocket;
  bool _scanNotificationSent = false;
  
  void scanNetworks(AsyncWebServerRequest* request);
  void listNetworks(AsyncWebServerRequest* request);
  void onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, 
                       AwsEventType type, void* arg, uint8_t* data, size_t len);
  void notifyScanComplete(int numNetworks, AsyncWebSocketClient* specificClient = nullptr);

#ifdef ESP8266
  uint8_t convertEncryptionType(uint8_t encryptionType);
#endif
};

#endif  // end WiFiScanner_h
