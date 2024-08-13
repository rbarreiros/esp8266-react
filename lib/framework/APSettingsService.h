#ifndef APSettingsConfig_h
#define APSettingsConfig_h

#include <SettingValue.h>
#include <HttpEndpoint.h>
#include <FSPersistence.h>
#include <JsonUtils.h>

#include <DNSServer.h>
#include <IPAddress.h>

#ifndef FACTORY_AP_PROVISION_MODE
#define FACTORY_AP_PROVISION_MODE AP_MODE_DISCONNECTED
#endif

#ifndef FACTORY_AP_SSID
#define FACTORY_AP_SSID "ESP8266-React-#{unique_id}"
#endif

#ifndef FACTORY_AP_PASSWORD
#define FACTORY_AP_PASSWORD "esp-react"
#endif

#ifndef FACTORY_AP_LOCAL_IP
#define FACTORY_AP_LOCAL_IP "192.168.4.1"
#endif

#ifndef FACTORY_AP_GATEWAY_IP
#define FACTORY_AP_GATEWAY_IP "192.168.4.1"
#endif

#ifndef FACTORY_AP_SUBNET_MASK
#define FACTORY_AP_SUBNET_MASK "255.255.255.0"
#endif

#ifndef FACTORY_AP_CHANNEL
#define FACTORY_AP_CHANNEL 1
#endif

#ifndef FACTORY_AP_SSID_HIDDEN
#define FACTORY_AP_SSID_HIDDEN false
#endif

#ifndef FACTORY_AP_MAX_CLIENTS
#define FACTORY_AP_MAX_CLIENTS 4
#endif

#define AP_SETTINGS_FILE "/config/apSettings.json"
#define AP_SETTINGS_SERVICE_PATH "/rest/apSettings"

#define AP_MODE_ALWAYS 0
#define AP_MODE_DISCONNECTED 1
#define AP_MODE_NEVER 2

#define MANAGE_NETWORK_DELAY 10000
#define DNS_PORT 53

#define APSETTINGS_MAX_SSID_SIZE  32
#define APSETTINGS_MAX_PASS_SIZE  64

enum APNetworkStatus { ACTIVE = 0, INACTIVE, LINGERING };

class APSettings 
{
 public:
  uint8_t provisionMode;
  char ssid[APSETTINGS_MAX_SSID_SIZE];
  char password[APSETTINGS_MAX_PASS_SIZE];
  uint8_t channel;
  bool ssidHidden;
  uint8_t maxClients;

  IPAddress localIP;
  IPAddress gatewayIP;
  IPAddress subnetMask;

  bool operator==(const APSettings& settings) const 
  {
    return provisionMode == settings.provisionMode && ssid == settings.ssid && password == settings.password &&
           channel == settings.channel && ssidHidden == settings.ssidHidden && maxClients == settings.maxClients &&
           localIP == settings.localIP && gatewayIP == settings.gatewayIP && subnetMask == settings.subnetMask;
  }

  static void read(APSettings& settings, JsonObject& root) 
  {
    root["provision_mode"] = settings.provisionMode;
    root["ssid"] = settings.ssid;
    root["password"] = settings.password;
    root["channel"] = settings.channel;
    root["ssid_hidden"] = settings.ssidHidden;
    root["max_clients"] = settings.maxClients;
    JsonUtils::writeIP(root, "local_ip", settings.localIP);
    JsonUtils::writeIP(root, "gateway_ip", settings.gatewayIP);
    JsonUtils::writeIP(root, "subnet_mask", settings.subnetMask);
  }

  static StateUpdateResult update(JsonObject& root, APSettings& settings) 
  {
    APSettings newSettings = {};
    newSettings.provisionMode = root["provision_mode"] | FACTORY_AP_PROVISION_MODE;
    
    switch (settings.provisionMode) 
    {
      case AP_MODE_ALWAYS:
      case AP_MODE_DISCONNECTED:
      case AP_MODE_NEVER:
        break;
      default:
        newSettings.provisionMode = AP_MODE_ALWAYS;
    };

    // THIS NEEDS FIXING, ELIMINATE ALL TRACES OF STRING
    if(root["ssid"].as<const char*>())
      strncpy(newSettings.ssid, root["ssid"].as<const char*>(), APSETTINGS_MAX_SSID_SIZE);
    else
      strncpy(newSettings.ssid, SettingValue::format(FACTORY_AP_SSID), APSETTINGS_MAX_SSID_SIZE); 
      //strncpy(newSettings.ssid, SettingValue::format(FACTORY_AP_SSID), APSETTINGS_MAX_SSID_SIZE); TODO!!!!!!

    if(root["password"].as<const char*>())
      strncpy(newSettings.password, root["password"].as<const char*>(), APSETTINGS_MAX_PASS_SIZE);
    else
      strncpy(newSettings.password, FACTORY_AP_PASSWORD, APSETTINGS_MAX_PASS_SIZE);

    newSettings.channel = root["channel"] | FACTORY_AP_CHANNEL;
    newSettings.ssidHidden = root["ssid_hidden"] | FACTORY_AP_SSID_HIDDEN;
    newSettings.maxClients = root["max_clients"] | FACTORY_AP_MAX_CLIENTS;

    JsonUtils::readIP(root, "local_ip", newSettings.localIP, FACTORY_AP_LOCAL_IP);
    JsonUtils::readIP(root, "gateway_ip", newSettings.gatewayIP, FACTORY_AP_GATEWAY_IP);
    JsonUtils::readIP(root, "subnet_mask", newSettings.subnetMask, FACTORY_AP_SUBNET_MASK);

    if (newSettings == settings)
      return StateUpdateResult::UNCHANGED;

    settings = newSettings;
    return StateUpdateResult::CHANGED;
  }
};

class APSettingsService : public StatefulService<APSettings> 
{
 public:
  APSettingsService(AsyncWebServer* server, FS* fs, SecurityManager* securityManager);

  void begin();
  void loop();
  APNetworkStatus getAPNetworkStatus();

 private:
  HttpEndpoint<APSettings> _httpEndpoint;
  FSPersistence<APSettings> _fsPersistence;

  // for the captive portal
  DNSServer* _dnsServer;

  // for the mangement delay loop
  volatile unsigned long _lastManaged;
  volatile boolean _reconfigureAp;

  void reconfigureAP();
  void manageAP();
  void startAP();
  void stopAP();
  void handleDNS();
};

#endif  // end APSettingsConfig_h
