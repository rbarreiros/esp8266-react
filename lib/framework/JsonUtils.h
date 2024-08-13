#ifndef JsonUtils_h
#define JsonUtils_h

#include <Arduino.h>
#include <IPUtils.h>
#include <ArduinoJson.h>

class JsonUtils 
{
public:
  static void readIP(JsonObject& root, const char* key, IPAddress& ip, const char* def) 
  {
    IPAddress defaultIp = {};
    
    if (!defaultIp.fromString(def)) {
      defaultIp = INADDR_NONE;
    }

    readIP(root, key, ip, defaultIp);
  }

  static void readIP(JsonObject& root, const char* key, IPAddress& ip, const IPAddress& defaultIp = INADDR_NONE) 
  {
    if (!root[key].is<const char*>() || !ip.fromString(root[key].as<const char*>())) {
      ip = defaultIp;
    }
  }

  static void writeIP(JsonObject& root, const char* key, const IPAddress& ip) 
  {
    if (IPUtils::isSet(ip)) {
      char ipStr[16];  // Max length of IPv4 address string (xxx.xxx.xxx.xxx\0)
      snprintf(ipStr, sizeof(ipStr), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
      root[key] = ipStr;
    }
  }
};

#endif  // end JsonUtils