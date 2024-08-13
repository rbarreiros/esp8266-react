#ifndef ArduinoJsonJWT_H
#define ArduinoJsonJWT_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <libb64/cdecode.h>
#include <libb64/cencode.h>

#ifdef ESP32
#include <mbedtls/md.h>
#elif defined(ESP8266)
#include <bearssl/bearssl_hmac.h>
#endif

#define JWT_MAX_SIZE 128

class ArduinoJsonJWT 
{
 private:
  char _secret[JWT_MAX_SIZE];

  const char* JWT_HEADER = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
  const int JWT_HEADER_SIZE = strlen(JWT_HEADER);

  size_t sign(const char* value, size_t len, char* outstring);

  static size_t encode(const char* cstr, size_t len, char* outstring);
  static size_t decode(const char* cstr, size_t len, char* outstring);

 public:
  ArduinoJsonJWT(const char *secret);

  void setSecret(const char* secret);
  size_t getSecret(char* secret);

  size_t buildJWT(JsonObject& payload, char* out, size_t len);
  void parseJWT(const char* jwt, size_t len, JsonDocument& jsonDocument);
};

#endif
