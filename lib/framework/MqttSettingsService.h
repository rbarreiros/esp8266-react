#ifndef MqttSettingsService_h
#define MqttSettingsService_h

#include <StatefulService.h>
#include <HttpEndpoint.h>
#include <FSPersistence.h>
#include <espMqttClientAsync.h>
#include <SettingValue.h>

#ifndef FACTORY_MQTT_ENABLED
#define FACTORY_MQTT_ENABLED false
#endif

#ifndef FACTORY_MQTT_HOST
#define FACTORY_MQTT_HOST "test.mosquitto.org"
#endif

#ifndef FACTORY_MQTT_PORT
#define FACTORY_MQTT_PORT 1883
#endif

#ifndef FACTORY_MQTT_USERNAME
#define FACTORY_MQTT_USERNAME ""
#endif

#ifndef FACTORY_MQTT_PASSWORD
#define FACTORY_MQTT_PASSWORD ""
#endif

#ifndef FACTORY_MQTT_CLIENT_ID
#define FACTORY_MQTT_CLIENT_ID "#{platform}-#{unique_id}"
#endif

#ifndef FACTORY_MQTT_KEEP_ALIVE
#define FACTORY_MQTT_KEEP_ALIVE 16
#endif

#ifndef FACTORY_MQTT_CLEAN_SESSION
#define FACTORY_MQTT_CLEAN_SESSION true
#endif

#define MQTT_SETTINGS_FILE "/config/mqttSettings.json"
#define MQTT_SETTINGS_SERVICE_PATH "/rest/mqttSettings"

#define MQTT_RECONNECTION_DELAY 5000

#define MQTT_HOST_SIZE      128
#define MQTT_USERNAME_SIZE  64
#define MQTT_PASSWORD_SIZE  64
#define MQTT_CLIENTID_SIZE  128

class MqttSettings 
{
 public:
  // host and port - if enabled
  bool enabled;
  char host[MQTT_HOST_SIZE];
  uint16_t port;

  // username and password
  char username[MQTT_USERNAME_SIZE];
  char password[MQTT_PASSWORD_SIZE];

  // client id settings
  char clientId[MQTT_CLIENTID_SIZE];

  // connection settings
  uint16_t keepAlive;
  bool cleanSession;

  static void read(MqttSettings& settings, JsonObject& root) 
  {
    root["enabled"] = settings.enabled;
    root["host"] = settings.host;
    root["port"] = settings.port;
    root["username"] = settings.username;
    root["password"] = settings.password;
    root["client_id"] = settings.clientId;
    root["keep_alive"] = settings.keepAlive;
    root["clean_session"] = settings.cleanSession;
  }

  static StateUpdateResult update(JsonObject& root, MqttSettings& settings) 
  {
    settings.enabled = root["enabled"] | FACTORY_MQTT_ENABLED;
    
    const char* host = root["host"] | FACTORY_MQTT_HOST;
    strncpy(settings.host, host, MQTT_HOST_SIZE - 1);
    settings.host[MQTT_HOST_SIZE - 1] = '\0';
    
    settings.port = root["port"] | FACTORY_MQTT_PORT;
    
    const char* username = root["username"] | FACTORY_MQTT_USERNAME;
    strncpy(settings.username, username, MQTT_USERNAME_SIZE - 1);
    settings.username[MQTT_USERNAME_SIZE - 1] = '\0';
    
    const char* password = root["password"] | FACTORY_MQTT_PASSWORD;
    strncpy(settings.password, password, MQTT_PASSWORD_SIZE - 1);
    settings.password[MQTT_PASSWORD_SIZE - 1] = '\0';
    
    const char* clientId = root["client_id"] | FACTORY_MQTT_CLIENT_ID;
    strncpy(settings.clientId, clientId, MQTT_CLIENTID_SIZE - 1);
    settings.clientId[MQTT_CLIENTID_SIZE - 1] = '\0';
    
    settings.keepAlive = root["keep_alive"] | FACTORY_MQTT_KEEP_ALIVE;
    settings.cleanSession = root["clean_session"] | FACTORY_MQTT_CLEAN_SESSION;
    
    return StateUpdateResult::CHANGED;
  }
};

class MqttSettingsService : public StatefulService<MqttSettings> 
{
 public:
  MqttSettingsService(AsyncWebServer* server, FS* fs, SecurityManager* securityManager);
  ~MqttSettingsService();

  void begin();
  void loop();
  bool isEnabled();
  bool isConnected();
  const char* getClientId();
  espMqttClientTypes::DisconnectReason getDisconnectReason();
  espMqttClientAsync* getMqttClient();

 protected:
  void onConfigUpdated();

 private:
  HttpEndpoint<MqttSettings> _httpEndpoint;
  FSPersistence<MqttSettings> _fsPersistence;

  // Pointers to hold retained copies of the mqtt client connection strings.
  // This is required as AsyncMqttClient holds refrences to the supplied connection strings.
  // TODO -- Is this still needed with espMqttClient ?
  char* _retainedHost;
  char* _retainedClientId;
  char* _retainedUsername;
  char* _retainedPassword;

  // variable to help manage connection
  bool _reconfigureMqtt;
  unsigned long _disconnectedAt;

  // connection status
  espMqttClientTypes::DisconnectReason _disconnectReason;

  // the MQTT client instance
  espMqttClientAsync _mqttClient;

#ifdef ESP32
  void onStationModeGotIP(WiFiEvent_t event, WiFiEventInfo_t info);
  void onStationModeDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);
#elif defined(ESP8266)
  WiFiEventHandler _onStationModeDisconnectedHandler;
  WiFiEventHandler _onStationModeGotIPHandler;
  void onStationModeGotIP(const WiFiEventStationModeGotIP& event);
  void onStationModeDisconnected(const WiFiEventStationModeDisconnected& event);
#endif

  void onMqttConnect(bool sessionPresent);
  void onMqttDisconnect(espMqttClientTypes::DisconnectReason reason);
  void configureMqtt();
};

#endif  // end MqttSettingsService_h
