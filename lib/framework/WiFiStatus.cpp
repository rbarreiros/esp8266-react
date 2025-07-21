#include <WiFiStatus.h>
#include <MemoryManager.h>

WiFiStatus::WiFiStatus(AsyncWebServer* server, SecurityManager* securityManager) 
{
  server->on(WIFI_STATUS_SERVICE_PATH,
             HTTP_GET,
             securityManager->wrapRequest(std::bind(&WiFiStatus::wifiStatus, this, std::placeholders::_1),
                                          AuthenticationPredicates::IS_AUTHENTICATED));
#ifdef ESP32
  // We want the device to come up in all circumstances so defaulting to 
  // WiFi.mode(WIFI_STA) such that the device reconnects when the WiFi is broken
  WiFi.mode(WIFI_STA);
  WiFi.onEvent(std::bind(&WiFiStatus::onStationModeGotIP, this, std::placeholders::_1, std::placeholders::_2),
               WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(std::bind(&WiFiStatus::onStationModeDisconnected, this, std::placeholders::_1, std::placeholders::_2),
               WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
#elif defined(ESP8266)
  _onStationModeConnectedHandler = WiFi.onStationModeConnected(std::bind(&WiFiStatus::onStationModeConnected, this, std::placeholders::_1));
  _onStationModeDisconnectedHandler = WiFi.onStationModeDisconnected(std::bind(&WiFiStatus::onStationModeDisconnected, this, std::placeholders::_1));
  _onStationModeGotIPHandler = WiFi.onStationModeGotIP(std::bind(&WiFiStatus::onStationModeGotIP, this, std::placeholders::_1));
#endif
}

#ifdef ESP32
void WiFiStatus::onStationModeGotIP(WiFiEvent_t event, WiFiEventInfo_t info) 
{
  Serial.printf_P(
      PSTR("WiFi Got IP. localIP=%s, hostName=%s\r\n"), WiFi.localIP().toString().c_str(), WiFi.getHostname());
}

void WiFiStatus::onStationModeDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) 
{
  Serial.printf_P(PSTR("WiFi Disconnected. Reason code=%d\r\n"), info.wifi_sta_disconnected.reason);
}
#elif defined(ESP8266)
void WiFiStatus::onStationModeConnected(const WiFiEventStationModeConnected& event) 
{
  Serial.printf_P(PSTR("WiFi Connected. SSID=%s\r\n"), event.ssid.c_str());
}

void WiFiStatus::onStationModeDisconnected(const WiFiEventStationModeDisconnected& event) 
{
  Serial.printf_P(PSTR("WiFi Disconnected. Reason code=%d\r\n"), event.reason);
}

void WiFiStatus::onStationModeGotIP(const WiFiEventStationModeGotIP& event) 
{
  Serial.printf_P(
      PSTR("WiFi Got IP. localIP=%s, hostName=%s\r\n"), event.ip.toString().c_str(), WiFi.hostname().c_str());
}
#endif

void WiFiStatus::wifiStatus(AsyncWebServerRequest* request) 
{
  wl_status_t status = WiFi.status();
  
  // Generate ETag based on connection status and IP
  String etag = "\"wifi-" + String((int)status);
  if (status == WL_CONNECTED) {
    etag += "-" + WiFi.localIP().toString();
  }
  etag += "\"";
  
  // Check if client has cached version
  if (request->hasHeader("If-None-Match")) {
    String clientEtag = request->getHeader("If-None-Match")->value();
    if (clientEtag == etag) {
      // Client has cached version, return 304 Not Modified
      request->send(304);
      return;
    }
  }
  
  // Use standard JSON response
  AsyncJsonResponse* response = new AsyncJsonResponse(false);
  JsonObject root = response->getRoot();
  
  root["status"] = (uint8_t)status;
  
  if (status == WL_CONNECTED) 
  {
    root["ip"] = WiFi.localIP().toString();
    root["mac"] = WiFi.macAddress();
    root["rssi"] = WiFi.RSSI();
    root["ssid"] = WiFi.SSID();
    root["bssid"] = WiFi.BSSIDstr();
    root["ch"] = WiFi.channel();
    root["subnet"] = WiFi.subnetMask().toString();
    root["gateway"] = WiFi.gatewayIP().toString();
    
    IPAddress dnsIP1 = WiFi.dnsIP(0);
    IPAddress dnsIP2 = WiFi.dnsIP(1);
    if (IPUtils::isSet(dnsIP1)) {
      root["dns1"] = dnsIP1.toString();
    }
    if (IPUtils::isSet(dnsIP2)) {
      root["dns2"] = dnsIP2.toString();
    }
  }
  
  // Send response with conditional caching
  response->addHeader("Cache-Control", "max-age=10, must-revalidate");  // Cache for 10 seconds
  response->addHeader("ETag", etag);
  response->setLength();
  request->send(response);
}
