#include <APStatus.h>
#include <JsonUtils.h>

APStatus::APStatus(AsyncWebServer* server, 
                   SecurityManager* securityManager, 
                   APSettingsService* apSettingsService) 
  :
  _apSettingsService{apSettingsService} 
{
  server->on(AP_STATUS_SERVICE_PATH,
             HTTP_GET,
             securityManager->wrapRequest(std::bind(&APStatus::apStatus, this, std::placeholders::_1),
                                          AuthenticationPredicates::IS_AUTHENTICATED));
}

void APStatus::apStatus(AsyncWebServerRequest* request) 
{
  AsyncJsonResponse* response = new AsyncJsonResponse(false);
  JsonObject root = response->getRoot();

  root["status"] = _apSettingsService->getAPNetworkStatus();
  JsonUtils::writeIP(root, "ip_address", WiFi.softAPIP());
  root["mac_address"] = WiFi.softAPmacAddress();
  root["station_num"] = WiFi.softAPgetStationNum();

  response->setLength();
  request->send(response);
}
