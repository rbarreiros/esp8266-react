#include <WiFiScanner.h>

WiFiScanner::WiFiScanner(AsyncWebServer* server, SecurityManager* securityManager) :
  _server(server), 
  _securityManager(securityManager),
  _scanWebSocket(new AsyncWebSocket("/ws/wifiScan"))
{
  server->on(SCAN_NETWORKS_SERVICE_PATH,
             HTTP_GET,
             securityManager->wrapRequest(std::bind(&WiFiScanner::scanNetworks, this, std::placeholders::_1),
                                          AuthenticationPredicates::IS_ADMIN));
  server->on(LIST_NETWORKS_SERVICE_PATH,
             HTTP_GET,
             securityManager->wrapRequest(std::bind(&WiFiScanner::listNetworks, this, std::placeholders::_1),
                                          AuthenticationPredicates::IS_ADMIN));
  
  // Set up WebSocket for scan completion notifications
  _scanWebSocket->setFilter(securityManager->filterRequest(AuthenticationPredicates::IS_ADMIN));
  _scanWebSocket->onEvent(std::bind(&WiFiScanner::onWebSocketEvent, this, 
                                   std::placeholders::_1, std::placeholders::_2, 
                                   std::placeholders::_3, std::placeholders::_4, 
                                   std::placeholders::_5, std::placeholders::_6));
  server->addHandler(_scanWebSocket.get());
}

void WiFiScanner::scanNetworks(AsyncWebServerRequest* request) 
{
  if (WiFi.scanComplete() != -1) {
    WiFi.scanDelete();
    WiFi.scanNetworks(true);
  }

  request->send(202);
}

void WiFiScanner::listNetworks(AsyncWebServerRequest* request) 
{
  int numNetworks = WiFi.scanComplete();
  
  if (numNetworks > -1) 
  {
    // Create standard JSON response to avoid manual string concatenation issues
    AsyncJsonResponse* response = new AsyncJsonResponse(false);
    JsonObject root = response->getRoot();
    
    root["count"] = numNetworks;
    JsonArray networks = root["networks"].to<JsonArray>();
    
    for (int i = 0; i < numNetworks; i++) 
    {
      JsonObject network = networks.add<JsonObject>();
      
      // Use compact field names to reduce payload size
      network["r"] = WiFi.RSSI(i);           // rssi
      network["s"] = WiFi.SSID(i);           // ssid  
      network["b"] = WiFi.BSSIDstr(i);       // bssid
      network["c"] = WiFi.channel(i);        // channel
      
      // encryption type
#ifdef ESP32
      network["e"] = (uint8_t)WiFi.encryptionType(i);
#elif defined(ESP8266)
      network["e"] = convertEncryptionType(WiFi.encryptionType(i));
#endif
    }
    
    // Add caching headers
    response->addHeader("Cache-Control", "max-age=30");
    response->setLength();
    request->send(response);
    
    // Notify WebSocket clients that scan is complete
    notifyScanComplete(numNetworks);
  } else if (numNetworks == -1) {
    request->send(202);
  } else {
    scanNetworks(request);
  }
}

void WiFiScanner::loop() 
{
  // Check for scan completion and notify WebSocket clients
  int scanResult = WiFi.scanComplete();
  if (scanResult > -1 && !_scanNotificationSent) {
    notifyScanComplete(scanResult);
    _scanNotificationSent = true;
  }
  
  // Reset notification flag when new scan starts
  if (scanResult == -1) {
    _scanNotificationSent = false;
  }
}

void WiFiScanner::onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, 
                                  AwsEventType type, void* arg, uint8_t* data, size_t len) 
{
  if (type == WS_EVT_CONNECT) {
    Serial.println("WiFi scan WebSocket client connected");
    
    // Send current scan status to new client
    int scanResult = WiFi.scanComplete();
    if (scanResult > -1) {
      notifyScanComplete(scanResult, client);
    } else {
      // Send scan in progress notification
      JsonDocument json;
      JsonObject root = json.to<JsonObject>();
      root["type"] = "scan_progress";
      root["message"] = "Scan in progress";
      
      size_t len = measureJson(json);
      AsyncWebSocketMessageBuffer* buffer = _scanWebSocket->makeBuffer(len);
      if (buffer) {
        serializeJson(json, buffer->get(), len);
        client->text(buffer);
      }
    }
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("WiFi scan WebSocket client disconnected");
  }
}

void WiFiScanner::notifyScanComplete(int numNetworks, AsyncWebSocketClient* specificClient) 
{
  JsonDocument json;
  JsonObject root = json.to<JsonObject>();
  root["type"] = "scan_complete";
  root["count"] = numNetworks;
  root["message"] = "Scan completed successfully";
  
  size_t len = measureJson(json);
  AsyncWebSocketMessageBuffer* buffer = _scanWebSocket->makeBuffer(len);
  if (buffer) {
    serializeJson(json, buffer->get(), len);
    if (specificClient) {
      specificClient->text(buffer);
    } else {
      _scanWebSocket->textAll(buffer);
    }
  }
}

#ifdef ESP8266
uint8_t WiFiScanner::convertEncryptionType(uint8_t encryptionType) 
{
  switch (encryptionType) 
  {
    case ENC_TYPE_NONE:
      return 0;
    case ENC_TYPE_WEP:
      return 1;
    case ENC_TYPE_TKIP:
      return 2;
    case ENC_TYPE_CCMP:
      return 3;
    case ENC_TYPE_AUTO:
      return 4;
    default:
      return 0;
  }
}
#endif
