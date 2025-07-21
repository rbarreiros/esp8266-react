#include <APSettingsService.h>

APSettingsService::APSettingsService(AsyncWebServer* server, FS* fs, SecurityManager* securityManager) 
  :
    _httpEndpoint{
      APSettings::read, 
      APSettings::update, 
      this, 
      server, 
      AP_SETTINGS_SERVICE_PATH, 
      securityManager
    },
    _fsPersistence
    {
      APSettings::read, 
      APSettings::update, 
      this, 
      fs, 
      AP_SETTINGS_FILE
    },
    _dnsServer{nullptr},
    _lastManaged{0},
    _reconfigureAp{false} 
{
  addUpdateHandler([&](const String& originId) { reconfigureAP(); }, false);
}

void APSettingsService::begin() 
{
  _fsPersistence.readFromFS();
  reconfigureAP();
}

void APSettingsService::reconfigureAP() 
{
  _lastManaged = millis() - MANAGE_NETWORK_DELAY;
  _reconfigureAp = true;
}

void APSettingsService::loop() 
{
  unsigned long currentMillis = millis();
  unsigned long manageElapsed = (unsigned long)(currentMillis - _lastManaged);

  if (manageElapsed >= MANAGE_NETWORK_DELAY) 
  {
    _lastManaged = currentMillis;
    manageAP();
  }
  handleDNS();
}

void APSettingsService::manageAP() 
{
  WiFiMode_t currentWiFiMode = WiFi.getMode();
  bool apCurrentlyRunning = (currentWiFiMode == WIFI_AP || currentWiFiMode == WIFI_AP_STA);
  bool wifiConnected = WiFi.isConnected();

  // Determine if AP should be running based on provision mode
  bool shouldRunAP = false;
  
  switch (_state.provisionMode) {
    case AP_MODE_ALWAYS:
      shouldRunAP = true;
      Serial.printf("AP: AP_MODE_ALWAYS - AP should be running\n");
      break;
      
    case AP_MODE_DISCONNECTED:
      shouldRunAP = !wifiConnected;
      Serial.printf("AP: AP_MODE_DISCONNECTED - AP should be %s (WiFi connected: %s)\n", 
                    shouldRunAP ? "running" : "stopped", wifiConnected ? "yes" : "no");
      break;
      
    case AP_MODE_NEVER:
      shouldRunAP = false;
      Serial.printf("AP: AP_MODE_NEVER - AP should never run\n");
      break;
      
    default:
      shouldRunAP = false;
      Serial.printf("AP: Unknown provision mode %d - defaulting to stop\n", _state.provisionMode);
      break;
  }

  // Take action based on desired vs current state
  if (shouldRunAP && (!apCurrentlyRunning || _reconfigureAp)) {
    Serial.printf("AP: Starting AP (mode=%d, status=%d, reconfigure=%s)\n", 
                  currentWiFiMode, WiFi.status(), _reconfigureAp ? "yes" : "no");
    startAP();
  } 
  else if (!shouldRunAP && apCurrentlyRunning) {
    // For AP_MODE_DISCONNECTED, only stop if no clients are connected (graceful shutdown)
    // For AP_MODE_NEVER, stop immediately regardless of clients
    bool forceStop = (_state.provisionMode == AP_MODE_NEVER) || _reconfigureAp;
    bool noClients = (WiFi.softAPgetStationNum() == 0);
    
    if (forceStop || noClients) {
      Serial.printf("AP: Stopping AP (mode=%d, clients=%d, connected=%s, force=%s)\n", 
                    currentWiFiMode, WiFi.softAPgetStationNum(), 
                    wifiConnected ? "yes" : "no", forceStop ? "yes" : "no");
      stopAP();
    } else {
      Serial.printf("AP: Delaying AP stop - %d clients still connected\n", WiFi.softAPgetStationNum());
    }
  }

  _reconfigureAp = false;
}

void APSettingsService::startAP() 
{
  Serial.println(F("Starting software access point"));
  
  // Set appropriate WiFi mode - preserve STA if it's active
  WiFiMode_t currentMode = WiFi.getMode();
  if (currentMode == WIFI_STA) {
    WiFi.mode(WIFI_AP_STA);
  } else if (currentMode == WIFI_OFF) {
    WiFi.mode(WIFI_AP);
  }
  // If already in AP or AP_STA mode, keep it as is
  
  WiFi.softAPConfig(_state.localIP, _state.gatewayIP, _state.subnetMask);
  
  WiFi.softAP(_state.ssid.c_str(), _state.password.c_str(), 
              _state.channel, _state.ssidHidden, _state.maxClients);

  if (!_dnsServer) 
  {
    IPAddress apIp = WiFi.softAPIP();
    Serial.print(F("Starting captive portal on "));
    Serial.println(apIp);
    _dnsServer = new DNSServer;
    _dnsServer->start(DNS_PORT, "*", apIp);
  }
}

void APSettingsService::stopAP() 
{
  if (_dnsServer) 
  {
    Serial.println(F("Stopping captive portal"));
    _dnsServer->stop();
    delete _dnsServer;
    _dnsServer = nullptr;
  }

  Serial.println(F("Stopping software access point"));
  WiFi.softAPdisconnect(true);
  
  // If we're in AP_STA mode and connected to WiFi, switch to pure STA
  if (WiFi.getMode() == WIFI_AP_STA && WiFi.isConnected()) {
    Serial.println(F("Switching to STA mode - WiFi connected and no AP clients"));
    WiFi.mode(WIFI_STA);
  }
}

void APSettingsService::handleDNS() 
{
  if (_dnsServer)
    _dnsServer->processNextRequest();
}

APNetworkStatus APSettingsService::getAPNetworkStatus() 
{
  WiFiMode_t currentWiFiMode = WiFi.getMode();
  bool apActive = currentWiFiMode == WIFI_AP || currentWiFiMode == WIFI_AP_STA;

  if (apActive && _state.provisionMode != AP_MODE_ALWAYS && WiFi.status() == WL_CONNECTED)
    return APNetworkStatus::LINGERING;

  return apActive ? APNetworkStatus::ACTIVE : APNetworkStatus::INACTIVE;
}
