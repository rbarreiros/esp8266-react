#include <FactoryResetService.h>

using namespace std::placeholders;

FactoryResetService::FactoryResetService(
    AsyncWebServer* server, FS* fs, 
    SecurityManager* securityManager
  ) 
  : fs{fs} 
{
  server->on(FACTORY_RESET_SERVICE_PATH,
             HTTP_POST,
             securityManager->wrapRequest(std::bind(&FactoryResetService::handleRequest, this, _1),
                                          AuthenticationPredicates::IS_ADMIN));
}

void FactoryResetService::handleRequest(
    AsyncWebServerRequest* request
  ) 
{
  request->onDisconnect(std::bind(&FactoryResetService::factoryReset, this));
  request->send(200);
}

/**
 * Delete function assumes that all files are stored flat, within the config directory.
 */
void FactoryResetService::factoryReset() 
{
#ifdef ESP32
  File root = fs->open(FS_CONFIG_DIRECTORY);
  File file;
  while (file = root.openNextFile()) {
    // Should remove this even for ESP32 ? 
    // To be decided....
    String path = file.path();
    file.close();
    fs->remove(path);
  }
#elif defined(ESP8266)
  Dir configDirectory = fs->openDir(FS_CONFIG_DIRECTORY);
  while (configDirectory.next()) 
  {
    int len = strlen(FS_CONFIG_DIRECTORY) + configDirectory.fileName().length() + 2;   
    char path[len];

    snprintf(path, len, "%s/%s", FS_CONFIG_DIRECTORY, configDirectory.fileName().c_str());
    fs->remove(path);
  }
#endif
  RestartService::restartNow();
}
