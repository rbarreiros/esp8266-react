#ifndef SettingValue_h
#define SettingValue_h

#include <Arduino.h>

#define MAX_STRING_LENGTH 256

namespace SettingValue 
{
  extern const char* PLATFORM;
  char* format(const char* value);
};

#endif