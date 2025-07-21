#include <WiFiSettingsService.h>

WiFiSettingsService::WiFiSettingsService(AsyncWebServer* server, FS* fs, SecurityManager* securityManager) 
  :
    _httpEndpoint
    {
      WiFiSettings::read, 
      WiFiSettings::update, 
      this, 
      server, 
      WIFI_SETTINGS_SERVICE_PATH, 
      securityManager
    },
    _fsPersistence
    {
      WiFiSettings::read, 
      WiFiSettings::update, 
      this, 
      fs, 
      WIFI_SETTINGS_FILE
    },
    _lastConnectionAttempt{0},
    _connectionStartTime{0},
    _reconnectionDelay{WIFI_RECONNECTION_DELAY},
    _retryAttempts{0},
    _isConnecting{false},
    _forceReconnect{false}
#ifdef ESP32
    , _stopping{false}
#endif
{
  // Only setup update handler in constructor - no WiFi calls during static initialization
  addUpdateHandler([&](const String& originId) { reconfigureWiFiConnection(); }, false);
}

void WiFiSettingsService::begin() 
{
  // WiFi initialization moved here from constructor to avoid static initialization issues
  // We want the device to come up in opmode=0 (WIFI_OFF), when erasing the flash this is not the default.
  // If needed, we save opmode=0 before disabling persistence so the device boots with WiFi disabled in the future.
  if (WiFi.getMode() != WIFI_OFF) {
    WiFi.mode(WIFI_OFF);
  }

  // Disable WiFi config persistance and auto reconnect
  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
  
#ifdef ESP32
  // Init the wifi driver on ESP32
  WiFi.mode(WIFI_MODE_MAX);
  WiFi.mode(WIFI_MODE_NULL);
  WiFi.onEvent(
      std::bind(&WiFiSettingsService::onStationModeDisconnected, this, std::placeholders::_1, std::placeholders::_2),
      WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  WiFi.onEvent(std::bind(&WiFiSettingsService::onStationModeStop, this, std::placeholders::_1, std::placeholders::_2),
               WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_STOP);
#elif defined(ESP8266)
  _onStationModeDisconnectedHandler = WiFi.onStationModeDisconnected(
      std::bind(&WiFiSettingsService::onStationModeDisconnected, this, std::placeholders::_1));
#endif

  _fsPersistence.readFromFS();
  resetConnectionState();
  reconfigureWiFiConnection();
}

void WiFiSettingsService::reconfigureWiFiConnection() 
{
  Serial.println(F("Reconfiguring WiFi connection"));
  
  // Stop current connection attempt
  if (_isConnecting) {
    WiFi.disconnect(true);
    _isConnecting = false;
  }
  
  // Reset connection state and force immediate reconnection
  resetConnectionState();
  _forceReconnect = true;
  _lastConnectionAttempt = 0;

#ifdef ESP32
  // For ESP32, we need to handle stopping state
  if (WiFi.disconnect(true)) {
    _stopping = true;
  }
#elif defined(ESP8266)
  // For ESP8266, just disconnect
  WiFi.disconnect(true);
#endif
}

void WiFiSettingsService::loop() 
{
  unsigned long currentMillis = millis();
  
  // Handle connection timeout
  if (_isConnecting) {
    handleConnectionTimeout();
  }
  
  // Check if it's time to attempt connection
  if (_forceReconnect || (!_lastConnectionAttempt || 
      (currentMillis - _lastConnectionAttempt) >= _reconnectionDelay)) {
    
    _lastConnectionAttempt = currentMillis;
    _forceReconnect = false;
    manageSTA();
  }
}

void WiFiSettingsService::manageSTA() 
{
  // Don't attempt connection if no SSID configured
  if (_state.ssid.length() == 0) {
    return;
  }
  
  // If already connected, reset connection state
  if (WiFi.isConnected()) {
    if (_isConnecting) {
      Serial.println(F("WiFi connected successfully"));
      resetConnectionState();
      
      // Optionally switch to pure STA mode if AP is not needed
      // (This depends on AP settings - let AP service manage this)
    }
    return;
  }
  
  // Check if we've exceeded maximum retry attempts
  if (_retryAttempts >= WIFI_MAX_RETRY_ATTEMPTS) {
    Serial.println(F("Max WiFi retry attempts reached. Waiting longer..."));
    _reconnectionDelay = WIFI_RECONNECTION_DELAY_MAX;
    _retryAttempts = 0;
    return;
  }
  
  // Don't start new connection if already connecting
  if (_isConnecting) {
    return;
  }
  
  Serial.printf("Attempting WiFi connection (attempt %d/%d)\n", _retryAttempts + 1, WIFI_MAX_RETRY_ATTEMPTS);
  
  // Configure WiFi mode - use AP_STA if AP is active, otherwise pure STA
  WiFiMode_t currentMode = WiFi.getMode();
  if (currentMode == WIFI_AP || currentMode == WIFI_AP_STA) {
    // Keep AP running while trying to connect to STA
    WiFi.mode(WIFI_AP_STA);
  } else {
    WiFi.mode(WIFI_STA);
  }
  
  // Configure IP settings
  if (_state.staticIPConfig) {
    Serial.println(F("Configuring static IP"));
    WiFi.config(_state.localIP, _state.gatewayIP, _state.subnetMask, _state.dnsIP1, _state.dnsIP2);
  } else {
    Serial.println(F("Configuring DHCP"));
#ifdef ESP32
    // For ESP32, set hostname before connecting
    WiFi.setHostname(_state.hostname.c_str());
#elif defined(ESP8266)
    // For ESP8266, reset IP config to DHCP mode
    WiFi.config(INADDR_ANY, INADDR_ANY, INADDR_ANY);
    WiFi.hostname(_state.hostname);
#endif
  }
  
  // Start connection attempt
  WiFi.begin(_state.ssid.c_str(), _state.password.c_str());
  
  // Update connection state
  _isConnecting = true;
  _connectionStartTime = millis();
  _retryAttempts++;
  
  // Calculate next backoff delay
  _reconnectionDelay = calculateBackoffDelay();
  
  Serial.printf("WiFi connection attempt started. Current mode: %d\n", WiFi.getMode());
}

void WiFiSettingsService::resetConnectionState() 
{
  _isConnecting = false;
  _connectionStartTime = 0;
  _retryAttempts = 0;
  _reconnectionDelay = WIFI_RECONNECTION_DELAY;
}

void WiFiSettingsService::handleConnectionTimeout() 
{
  unsigned long currentMillis = millis();
  
  if (currentMillis - _connectionStartTime >= WIFI_CONNECTION_TIMEOUT) {
    Serial.println(F("WiFi connection timeout"));
    
    // Stop current connection attempt
    WiFi.disconnect(true);
    _isConnecting = false;
    
    // Connection failed, manageSTA will handle retry on next loop
  }
}

unsigned long WiFiSettingsService::calculateBackoffDelay() 
{
  // Exponential backoff with jitter: delay = base * (2^attempts) + random(0, 1000)
  unsigned long baseDelay = WIFI_RECONNECTION_DELAY;
  unsigned int cappedRetries = (_retryAttempts > 6U) ? 6U : _retryAttempts; // Cap at 2^6 = 64
  unsigned long backoffDelay = baseDelay * (1UL << cappedRetries);
  
  // Add jitter to prevent thundering herd
  backoffDelay += random(0, 1000);
  
  // Cap at maximum delay
  unsigned long maxDelay = WIFI_RECONNECTION_DELAY_MAX;
  return (backoffDelay > maxDelay) ? maxDelay : backoffDelay;
}

#ifdef ESP32
void WiFiSettingsService::onStationModeDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.printf("WiFi disconnected. Reason: %d\n", info.wifi_sta_disconnected.reason);
  
  // Reset connection state if we were connecting
  if (_isConnecting) {
    _isConnecting = false;
  }
  
  // Don't call WiFi.disconnect() here - we're already disconnected!
  // Just reset the connection attempt timer to trigger reconnection
  _lastConnectionAttempt = 0;
}

void WiFiSettingsService::onStationModeStop(WiFiEvent_t event, WiFiEventInfo_t info) {
  if (_stopping) {
    Serial.println(F("WiFi stopped"));
    _lastConnectionAttempt = 0;
    _stopping = false;
  }
}
#elif defined(ESP8266)
void WiFiSettingsService::onStationModeDisconnected(const WiFiEventStationModeDisconnected& event) 
{
  Serial.printf("WiFi disconnected. Reason: %d\n", event.reason);
  
  // Reset connection state if we were connecting
  if (_isConnecting) {
    _isConnecting = false;
  }
  
  // Don't call WiFi.disconnect() here - we're already disconnected!
  // Just reset the connection attempt timer to trigger reconnection
  _lastConnectionAttempt = 0;
}
#endif
