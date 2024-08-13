#include <SettingValue.h>
#include <ESP8266WiFi.h>

namespace SettingValue 
{

#ifdef ESP32
const char* PLATFORM = "esp32";
#elif defined(ESP8266)
const char* PLATFORM = "esp8266";
#endif

char* replaceEach(const char* value, const char* pattern, const char* (*generateReplacement)()) 
{
  static char result[MAX_STRING_LENGTH];
  char* dst = result;
  const char* src = value;
  size_t patternLen = strlen(pattern);

  while (*src && (dst - result) < MAX_STRING_LENGTH - 1) 
  {
    if (strncmp(src, pattern, patternLen) == 0) 
    {
      const char* replacement = generateReplacement();
      size_t replaceLen = strlen(replacement);
      if ((dst - result) + replaceLen < MAX_STRING_LENGTH - 1) 
      {
        strcpy(dst, replacement);
        dst += replaceLen;
        src += patternLen;
      } 
      else 
      {
        break;
      }
    } 
    else 
    {
      *dst++ = *src++;
    }
  }
  *dst = '\0';
  return result;
}

const char* getRandom() 
{
  static char randomStr[9];
  snprintf(randomStr, sizeof(randomStr), "%08lx", random(0xFFFFFFFF));
  return randomStr;
}

const char* getUniqueId() 
{
  static char macStr[13] = {0};
  if (macStr[0] == 0) 
  {
    uint8_t mac[6];
#ifdef ESP32
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
#elif defined(ESP8266)
    wifi_get_macaddr(STATION_IF, mac);
#endif
    snprintf(macStr, sizeof(macStr), "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  }
  return macStr;
}

char* format(const char* value) 
{
  static char result[MAX_STRING_LENGTH];
  strncpy(result, value, MAX_STRING_LENGTH - 1);
  result[MAX_STRING_LENGTH - 1] = '\0';

  char* temp = replaceEach(result, "#{random}", getRandom);
  strncpy(result, temp, MAX_STRING_LENGTH - 1);

  char* uniqueIdPos = strstr(result, "#{unique_id}");
  if (uniqueIdPos) 
  {
    size_t prefixLen = uniqueIdPos - result;
    const char* uniqueId = getUniqueId();
    snprintf(result + prefixLen, MAX_STRING_LENGTH - prefixLen, "%s%s", uniqueId, uniqueIdPos + 11);
  }

  char* platformPos = strstr(result, "#{platform}");
  if (platformPos) 
  {
    size_t prefixLen = platformPos - result;
    snprintf(result + prefixLen, MAX_STRING_LENGTH - prefixLen, "%s%s", PLATFORM, platformPos + 11);
  }

  return result;
}

};  // end namespace SettingValue