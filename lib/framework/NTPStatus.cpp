#include <NTPStatus.h>

NTPStatus::NTPStatus(AsyncWebServer* server, SecurityManager* securityManager) 
{
  server->on(NTP_STATUS_SERVICE_PATH,
             HTTP_GET,
             securityManager->wrapRequest(std::bind(&NTPStatus::ntpStatus, this, std::placeholders::_1),
                                          AuthenticationPredicates::IS_AUTHENTICATED));
}

/*
 * Formats the time using the format provided.
 *
 * Uses a 25 byte buffer, large enough to fit an ISO time string with offset.
 */
size_t formatTime(tm* time, const char* format, char* out) 
{
  return strftime(out, 25, format, time);
}

size_t toUTCTimeString(tm* time, char* out) 
{
  return formatTime(time, "%FT%TZ", out);
}

size_t toLocalTimeString(tm* time, char* out) 
{
  return formatTime(time, "%FT%T", out);
}

void NTPStatus::ntpStatus(AsyncWebServerRequest* request) 
{
  AsyncJsonResponse* response = new AsyncJsonResponse(false);
  JsonObject root = response->getRoot();
  char buffer[30];

  // grab the current instant in unix seconds
  time_t now = time(nullptr);

  // only provide enabled/disabled status for now
  root["status"] = sntp_enabled() ? 1 : 0;

  // the current time in UTC
  size_t len = toUTCTimeString(gmtime(&now), buffer);
  buffer[len] = '\0';
  root["utc_time"] = buffer;

  // local time with offset
  len = toLocalTimeString(localtime(&now), buffer);
  buffer[len] = '\0';
  root["local_time"] = buffer;

  // the sntp server name
  root["server"] = sntp_getservername(0);

  // device uptime in seconds
  root["uptime"] = millis() / 1000;

  response->setLength();
  request->send(response);
}
