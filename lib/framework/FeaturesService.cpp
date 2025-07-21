#include <FeaturesService.h>
#include <MemoryManager.h>

FeaturesService::FeaturesService(AsyncWebServer* server) 
{
  server->on(FEATURES_SERVICE_PATH, HTTP_GET, 
             std::bind(&FeaturesService::features, this, std::placeholders::_1)
            );
}

void FeaturesService::features(AsyncWebServerRequest* request) 
{
  // Check if client has cached version
  if (request->hasHeader("If-None-Match")) {
    String etag = request->getHeader("If-None-Match")->value();
    if (etag == "\"features-v1\"") {
      // Client has cached version, return 304 Not Modified
      request->send(304);
      return;
    }
  }
  
  // Use standard JSON response to avoid boolean parsing issues
  AsyncJsonResponse* response = new AsyncJsonResponse(false);
  JsonObject root = response->getRoot();
  
  // Compact feature flags - ensure boolean serialization
#if FT_ENABLED(FT_PROJECT)
  root["proj"] = true;
#else
  root["proj"] = false;
#endif

#if FT_ENABLED(FT_SECURITY)
  root["sec"] = true;
#else
  root["sec"] = false;
#endif

#if FT_ENABLED(FT_MQTT)
  root["mqtt"] = true;
#else
  root["mqtt"] = false;
#endif

#if FT_ENABLED(FT_NTP)
  root["ntp"] = true;
#else
  root["ntp"] = false;
#endif

#if FT_ENABLED(FT_OTA)
  root["ota"] = true;
#else
  root["ota"] = false;
#endif

#if FT_ENABLED(FT_UPLOAD_FIRMWARE)
  root["up_fw"] = true;
#else
  root["up_fw"] = false;
#endif
  
  // Send response with aggressive caching headers
  response->addHeader("Cache-Control", "public, max-age=3600, immutable");  // Cache for 1 hour
  response->addHeader("ETag", "\"features-v1\"");  // Simple ETag for feature set
  response->setLength();
  request->send(response);
}
